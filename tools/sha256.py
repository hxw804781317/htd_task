import argparse
import hashlib
import os
import sys

def calc_sha256(file_path: str, chunk_size: int = 1024 * 1024) -> str:
    """计算文件sha256，返回小写16进制字符串"""
    h = hashlib.sha256()
    try:
        with open(file_path, "rb") as f:
            while buf := f.read(chunk_size):
                h.update(buf)
    except OSError as e:
        print(f"读取文件错误：{e}", file=sys.stderr)
        sys.exit(1)
    return h.hexdigest()

def build_new_filepath(src_path: str, sha_hex: str) -> str:
    """
    原文件：filename.xxx
    新文件：filename-{sha256十六进制串}.xxx
    示例：firmware.bin → firmware-34ad2f......(64位sha).bin
    """
    folder = os.path.dirname(src_path)
    base_name = os.path.basename(src_path)
    name, ext = os.path.splitext(base_name)
    new_name = f"{name}-{sha_hex}{ext}"
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
    parser = argparse.ArgumentParser(description="复制文件，新文件名嵌入该文件的SHA256十六进制值")
    parser.add_argument("src_file", help="源文件路径，如 test.bin")
    args = parser.parse_args()

    src_path = args.src_file
    sha_val = calc_sha256(src_path)
    #print(f"源文件 SHA256: {sha_val}")

    dst_path = build_new_filepath(src_path, sha_val)
    print(f"output new file: {dst_path}")

    copy_file(src_path, dst_path)
    #print("完成，新文件内容与原文件完全相同")