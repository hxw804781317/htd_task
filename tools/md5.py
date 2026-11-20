import argparse
import hashlib
import os
import sys

def calc_md5(file_path: str, chunk_size: int = 1024 * 1024) -> str:
    """计算文件md5，返回小写16进制字符串"""
    h = hashlib.md5()
    try:
        with open(file_path, "rb") as f:
            while buf := f.read(chunk_size):
                h.update(buf)
    except OSError as e:
        print(f"读取文件错误：{e}", file=sys.stderr)
        sys.exit(1)
    return h.hexdigest()

def build_new_filepath(src_path: str, md5_hex: str) -> str:
    """
    原文件：filename.xxx
    新文件：filename-{md5十六进制串}.xxx
    示例：firmware.bin → firmware-d41d8cd98f00b204e9800998ecf8427e.bin
    """
    folder = os.path.dirname(src_path)
    base_name = os.path.basename(src_path)
    name, ext = os.path.splitext(base_name)
    new_name = f"{name}-{md5_hex}{ext}"
    return os.path.join(folder, new_name)

def copy_file(src: str, dst: str, chunk_size: int = 1024 * 1024):
    """二进制复制文件，内容完全一致"""
    try:
        with open(src, "rb") as fr, open(dst, "wb") as fw:
            while buf := fr.read(chunk_size):
                fw.write(buf)
    except OSError as e:
        print(f"写入目标文件错误：{e}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="复制文件，新文件名嵌入该文件的MD5十六进制值")
    parser.add_argument("src_file", help="源文件路径，如 test.bin")
    args = parser.parse_args()

    src_path = args.src_file
    md5_val = calc_md5(src_path)
    #print(f"源文件 MD5: {md5_val}")

    dst_path = build_new_filepath(src_path, md5_val)
    print(f"out new file: {dst_path}")

    copy_file(src_path, dst_path)
    #print("完成，新文件内容与原文件完全相同")