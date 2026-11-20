#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import argparse
import hashlib
from cryptography.hazmat.primitives.asymmetric import ec, utils
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.asymmetric.utils import (
    decode_dss_signature,
    encode_dss_signature,
)
from cryptography.exceptions import InvalidSignature


# 常量: secp256r1 的字节长度
PRIV_KEY_SIZE = 32
PUB_KEY_SIZE = 64


# ============================================================
# 生成公钥和私钥，对应 uECC_make_key
# ============================================================
def uecc_secp256r1_make_key():
    priv_obj = ec.generate_private_key(ec.SECP256R1())
    pub_obj = priv_obj.public_key()

    # 私钥 -> 32 字节大端序
    secp256r1_private_key = priv_obj.private_numbers().private_value.to_bytes(
        PRIV_KEY_SIZE, "big"
    )

    # 公钥 -> 64 字节 X || Y
    pub_nums = pub_obj.public_numbers()
    secp256r1_public_key = (
        pub_nums.x.to_bytes(32, "big") + pub_nums.y.to_bytes(32, "big")
    )
    return secp256r1_public_key, secp256r1_private_key



# 生成C数组格式
def _format_c_array(data: bytes) -> str:
    return ", ".join(f"0x{b:02X}" for b in data) + ","

# 将公钥和私钥保存到C头文件中
def uecc_save_key_to_header(secp256r1_private_key: bytes,
                            secp256r1_public_key: bytes,
                            filepath: str = "htd_boot_key.h") -> None:
    if len(secp256r1_private_key) != PRIV_KEY_SIZE:
        raise ValueError(f"secp256r1_private_key 必须为 {PRIV_KEY_SIZE} 字节")
    if len(secp256r1_public_key) != PUB_KEY_SIZE:
        raise ValueError(f"secp256r1_public_key 必须为 {PUB_KEY_SIZE} 字节")

    priv_body = _format_c_array(secp256r1_private_key)
    pub_body = _format_c_array(secp256r1_public_key)

    content = f"""#ifndef _HTD_BOOT_KEY_H_
#define _HTD_BOOT_KEY_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {{
#endif /*__cplusplus*/

#define HTD_BOOT_SECP256R1_PRIVATE_KEY {{{priv_body}}}
#define HTD_BOOT_SECP256R1_PUBLIC_KEY {{{pub_body}}}

#ifdef __cplusplus
}}
#endif /*__cplusplus*/
#endif /*_HTD_BOOT_KEY_H_*/

"""
    with open(filepath, "w", encoding="utf-8") as f:
        f.write(content)
    print(f"[+] 已写入 {filepath}")



def main():
    parser = argparse.ArgumentParser(description='Generate uECC-Key file.');
    parser.add_argument('-o', '--output', metavar="FILE", dest='output', type=str, help='write output to named file;');
    args = parser.parse_args();
    if args.output == None:
        args.output = "./htd_boot_key.h";

    # --- 生成密钥对 (uECC_make_key) ---
    secp256r1_public_key, secp256r1_private_key = uecc_secp256r1_make_key();
    uecc_save_key_to_header(secp256r1_private_key, secp256r1_public_key, args.output);

    print("\n[uecc_secp256r1_make_key]");
    print(f"  secp256r1_private_key ({len(secp256r1_private_key)}B): {secp256r1_private_key.hex()}");
    print(f"  secp256r1_public_key  ({len(secp256r1_public_key)}B): {secp256r1_public_key.hex()}");


if __name__ == "__main__":
    main()