# -*- coding: utf-8 -*-
import subprocess
import sys
import os

TEMPLATE_CONTENT = r"""#define VERSION_MAJOR            {MAJOR_T}
#define VERSION_MINOR            {MINOR_T}
#define VERSION_REVISION         {REVISION_T}
#define VERSION_BUILDNUM         {BUILDNUM_T}
#define VERSION_GCID             {GITCMTID_T}
#define VERSION_GCID2            {GITCMTID2_T}
#define NAME2STR(a)              #a
#define NUM4STR(a,b,c,d)         #a "." #b "." #c "." #d
#define VERSIONBUILDSTR(a,b,c,d) NUM4STR(a,b,c,d)
#define VERSION_BUILD_STR        VERSIONBUILDSTR(VERSION_MAJOR,VERSION_MINOR,VERSION_REVISION,VERSION_BUILDNUM)
"""

"""执行shell命令，返回stdout字符串，失败返回None"""
def run_cmd(cmd):
    try:
        p = subprocess.Popen(
            cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
            text=True, encoding="utf-8", errors="replace"
        )
        out, err = p.communicate()
        if p.returncode != 0:
            return None
        return out.strip()
    except Exception:
        return None

# 生成版本文件
def generate_version_file(tag, version_filepath):
    if not tag:
        tag = "v"
    else:
        tag = tag + "v"

    full_ver = run_cmd("git describe")
    if full_ver is None or full_ver == "":
        full_ver = tag + "0.0.0"
    idx = full_ver.find("v")
    if idx != -1:
        tag = full_ver[:idx+1]

    tag_len = len(tag)
    proj_version = full_ver[tag_len:]

    parts_dash = proj_version.split("-")
    tag_version = parts_dash[0] if len(parts_dash)>=1 else ""
    build_num_str = parts_dash[1] if len(parts_dash)>=2 else ""

    dot_parts = tag_version.split(".")
    major = dot_parts[0] if len(dot_parts)>=1 else ""
    minor = dot_parts[1] if len(dot_parts)>=2 else ""
    revison = dot_parts[2] if len(dot_parts)>=3 else ""

    if not major:
        major = "0"
    if not minor:
        minor = "0"
    if not revison:
        revison = "0"
    if not build_num_str:
        build_num = 0
    else:
        build_num = int(build_num_str)

    if build_num > 2048:
        print(f"Build number: {build_num}")
        print("Build number too large, Need to update revision !!!")
        sys.exit(-1)

    # git commit id
    cmt_line = run_cmd("git log --pretty=oneline -1")
    if cmt_line is None:
        cmt_raw = "0000000000000000"
    else:
        cmt_raw = cmt_line[:16]
    GIT_CMTID_RAW = cmt_raw[0:8]
    GIT_CMTID = f"0x{GIT_CMTID_RAW}"
    GIT_CMTID2_RAW = cmt_raw[8:16]
    GIT_CMTID2 = f"0x{GIT_CMTID2_RAW}"

    out_content = TEMPLATE_CONTENT.format(
        MAJOR_T=major,
        MINOR_T=minor,
        REVISION_T=revison,
        BUILDNUM_T=str(build_num),
        GITCMTID_T=GIT_CMTID,
        GITCMTID2_T=GIT_CMTID2,
    )
    with open(version_filepath, "w", encoding="utf-8") as f:
        f.write(out_content);

