#!/usr/bin/env python3
# coding=utf-8
import sys
import os
import re
import struct
from array import *
import argparse
from binascii import unhexlify
from pathlib import Path

import hashlib
from cryptography.hazmat.primitives.asymmetric import ec, utils
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.backends import default_backend
from cryptography.exceptions import InvalidSignature

import read_macro
HTD_BOOT_MAGIC = 0xae667182


class MacroKeyLoader:
    """从 #define NAME {0x.., 0x.., ...} 形式的头文件加载密钥"""
    _HEX_RE = re.compile(r'0x([0-9A-Fa-f]{1,2})')
    def __init__(self, header_path: str):
        self._path = Path(header_path)
        if not self._path.is_file():
            raise FileNotFoundError(f"文件不存在: {self._path}")
        self._text = self._path.read_text(encoding='utf-8', errors='ignore')

    @staticmethod
    def _strip_comments(text: str) -> str:
        """去掉 // 和 /* */ 注释"""
        text = re.sub(r'/\*.*?\*/', '', text, flags=re.S)
        text = re.sub(r'//[^\n]*', '', text)
        return text

    @staticmethod
    def _join_continuations(text: str) -> str:
        """合并反斜杠续行"""
        return re.sub(r'\\\s*\n', ' ', text)

    def load_macro_array(self, macro_name: str) -> bytes:
        """
        加载形如 #define NAME {0x.., 0x.., ...} 的宏，返回 bytes
        """
        text = self._join_continuations(self._strip_comments(self._text))

        # 匹配 #define NAME { ... }
        # [^}]* 抓花括号内部内容，避免跨宏误匹配
        pattern = re.compile(
            r'^\s*#\s*define\s+' + re.escape(macro_name) + r'\s*\{([^}]*)\}',
            re.MULTILINE,
        )
        m = pattern.search(text)
        if not m:
            raise ValueError(f"未找到宏: {macro_name}")

        body = m.group(1)
        hex_values = self._HEX_RE.findall(body)
        if not hex_values:
            raise ValueError(f"宏 {macro_name} 内未找到十六进制数据")
        return bytes(int(x, 16) for x in hex_values)
    def load_keys(self):
        """一次性加载私钥和公钥，返回 (priv, pub)"""
        priv = self.load_macro_array('HTD_BOOT_SECP256R1_PRIVATE_KEY')
        pub  = self.load_macro_array('HTD_BOOT_SECP256R1_PUBLIC_KEY')

        if len(priv) != 32:
            raise ValueError(f"私钥长度应为 32，实际 {len(priv)}")
        if len(pub) != 64:
            raise ValueError(f"公钥长度应为 64，实际 {len(pub)}")
        return priv, pub



""" 
对 32 字节摘要做 secp256r1 签名
参数:
    private_key: 32 字节裸私钥 (big-endian)
    data       : 32 字节摘要，不做任何哈希处理，直接参与 ECDSA 运算
返回:
    bytes: 签名
"""
def uecc_sign(private_key: bytes, data: bytes) -> bytes:
    if len(private_key) != 32:
        raise ValueError(f"secp256r1 私钥必须是32字节，实际{len(private_key)}")
    if len(data) != 32:
        raise ValueError(f"data 必须是32字节摘要，实际{len(data)}")

    priv_int = int.from_bytes(private_key, 'big')
    priv_key = ec.derive_private_key(priv_int, ec.SECP256R1(), default_backend())

    # Prehashed: 声明 data 已是摘要，库不再做哈希
    signature = priv_key.sign(data, ec.ECDSA(utils.Prehashed(hashes.SHA256())))
    r, s = utils.decode_dss_signature(signature)
    return r.to_bytes(32, 'big') + s.to_bytes(32, 'big')

""" 
uEcc 验证签名
参数:
    public_key : 64 字节裸公钥
    data       : 32 字节摘要
    signature  : raw -> 64字节
返回:
    bool
"""
def uecc_verify(public_key: bytes, data: bytes, signature: bytes) -> bool:
    if len(public_key) != 64:
        raise ValueError(f"secp256r1 裸公钥必须是 64 字节，实际 {len(public_key)}")
    if len(data) != 32:
        raise ValueError(f"data 必须是 32 字节摘要，实际 {len(data)}")

    point_bytes = b'\x04' + public_key
    pub_key = ec.EllipticCurvePublicKey.from_encoded_point(
        ec.SECP256R1(), point_bytes
    )
    
    if len(signature) != 64:
        raise ValueError(f"raw 签名必须是 64 字节，实际 {len(signature)}")
    r = int.from_bytes(signature[:32], 'big')
    s = int.from_bytes(signature[32:], 'big')
    der_sig = utils.encode_dss_signature(r, s)

    try:
        pub_key.verify(der_sig, data, ec.ECDSA(utils.Prehashed(hashes.SHA256())))
        return True
    except InvalidSignature:
        return False

"""
修改文件boot包头
typedef struct {
    uint32_t magic;                 // 魔数，固件和boot必须一致
    uint8_t verify_code[64];        // 使用私钥对sha256的值做签名，使用公钥验证
    uint32_t fw_size;               // 固件大小，单位字节，包含所有头部数据
    
    uint16_t fw_hdr_size;           // 固件头部大小，sizeof(boot_firmware_header_t)
    uint8_t update_type;            // 升级方式, 固件类型
    uint8_t reserve0;
    
    uint8_t ver_major;              // 主版本号
    uint8_t ver_minor;              // 次版本号
    uint8_t ver_revision;           // 修订版本
    uint8_t ver_reserve;            // 保留版本号
}boot_header_t;
#define HTD_BOOT_MAGIC              0xae667182
"""
def boot_head_change(file_data, out_file, private_key):
    # 校验魔数
    magic = struct.unpack_from('<I', file_data, 0)[0]
    if magic != HTD_BOOT_MAGIC:
        raise ValueError(f"magic: 0x0xae667182->0x{magic:08X}");
    # 文件大小
    total_size = len(file_data);
    if total_size <= 512:
        raise ValueError(f"file size is small. %d", total_size);
    struct.pack_into('<I', file_data, 68, total_size);
    # sha256
    data_sha256 = hashlib.sha256(file_data[68:]).digest();
    # 签名
    signature = uecc_sign(private_key, data_sha256);
    file_data[4:68] = signature;
    with open(out_file, 'wb') as f:
        f.write(file_data)


# 读宏内容
def read_macro_int(header_file: str, macro_name: str) -> int:
    """从 .h 文件读取宏，返回整数"""
    path = Path(header_file);
    if not path.is_file():
        raise FileNotFoundError(f"文件不存在: {path}");

    value, _ = read_macro.find_macro(path, macro_name);
    if value is None:
        raise KeyError(f"未找到宏 {macro_name} in {path}");

    parsed = read_macro.parse_value(value);
    if not isinstance(parsed, int):
        raise ValueError(f"宏 {macro_name} 的值不是整数: '{value}'");
    return parsed

# 生成头部
def boot_head_make(ver_file, update_type):
    ver_major = read_macro_int(ver_file, 'VERSION_MAJOR');
    ver_minor = read_macro_int(ver_file, 'VERSION_MINOR');
    ver_revision = read_macro_int(ver_file, 'VERSION_REVISION');
    ver_reserve = read_macro_int(ver_file, 'VERSION_BUILDNUM');
    buf = bytearray(80);
    struct.pack_into('<I', buf, 0, HTD_BOOT_MAGIC);
    struct.pack_into('<I', buf, 68, 0);
    struct.pack_into('<H', buf, 72, 80);
    buf[74] = update_type & 0xFF;
    buf[76] = ver_major;
    buf[77] = ver_minor;
    buf[78] = ver_revision;
    buf[79] = ver_reserve;
    print("ver: %02x.%02x.%02x.%02x" % (ver_major, ver_minor, ver_revision, ver_reserve));
    return buf;
    
# 主函数
if __name__ == '__main__':
    parser = argparse.ArgumentParser();
    parser.add_argument('-i', '--input', metavar="FILE", dest='input', type=str, required=True, help='输入某个固件文件');
    parser.add_argument('-o', '--output', metavar="FILE", dest='output', type=str, required=True, help='生成文件名字');
    parser.add_argument('-k', '--key', metavar="FILE", dest='key', type=str, required=True, help='输入秘钥文件');
    parser.add_argument('-m', '--make', metavar="MODE", dest='make', type=int, choices=[1, 2], required=True, help='1:修改包头, 2:添加包头');
    parser.add_argument('-v', '--ver', metavar="FILE", dest='ver', type=str, help='版本文件');
    parser.add_argument('-t', '--type', metavar="UPDATE_TYPE", dest="type", type=int, choices=[3, 4], help='3:quicklz; 4:fastlz');
    
    FLAGS = parser.parse_args()
    print(FLAGS)
    print(f"输入: {FLAGS.input}");
    print(f"输出: {FLAGS.output}");
    print(f"秘钥: {FLAGS.key}");
    print(f"模式: {FLAGS.make}");
    
    # 提取秘钥文件中的私钥
    key_loader = MacroKeyLoader(FLAGS.key);
    private_key, public_key = key_loader.load_keys();
    #print(f"private_key={private_key}");
    #print(f"public_key={public_key}");
    
    # 导入文件
    with open(FLAGS.input, 'rb') as f:
        file_data = bytearray(f.read());
    if FLAGS.make != 1:
        head_data = boot_head_make(FLAGS.ver, FLAGS.type);
        head_data += file_data;
        file_data = head_data;
    boot_head_change(file_data, FLAGS.output, private_key);