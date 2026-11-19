// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL 4
#define DEBUG_TAG              "main"
#include "htd_task.h"
#include "app_board.h"
#include "app_file.h"
#include "htd_device.h"
#include "htd_components.h"


#define COMPRESS_BUFFER_SIZE            4096
#define DECOMPRESS_BUFFER_SIZE          4096

#if 0
//////////////////////////////////////////////////////////////////////
#include "quicklz.h"
#define QLZ_BUFFER_PADDING              400

// @brief 压缩文件
void quicklz_file_compress(const char* in_filepath, const char* out_filepath)
{
    file_fd_t infile_handle = app_file_open(in_filepath, O_BINARY|O_RDONLY);
    if (infile_handle == NULL) {
        print_error("open file(%s) failed.", in_filepath);
        return;
    }
    uint32_t file_size = app_file_size(infile_handle);
    
    file_fd_t outfile_handle = app_file_open(out_filepath, O_BINARY|O_WRONLY|O_CREAT|O_TRUNC);
    if (outfile_handle == NULL) {
        app_file_close(infile_handle);
        print_error("open file(%s) failed.", out_filepath);
        return ;
    }
    
    uint8_t* compress_buff = (uint8_t *)htd_malloc(COMPRESS_BUFFER_SIZE + QLZ_BUFFER_PADDING + sizeof(uint32_t));
    ASSERT(compress_buff);
    uint8_t* buff = (uint8_t *)htd_malloc(COMPRESS_BUFFER_SIZE);
    ASSERT(buff);
    qlz_state_compress *state_compress = (qlz_state_compress*)htd_malloc(sizeof(qlz_state_compress));
    ASSERT(state_compress);
    memset(state_compress, 0x00, sizeof(qlz_state_compress));

    uint32_t totle_cmprs_size = 0;
    uint32_t compress_len;
    uint32_t buff_len = COMPRESS_BUFFER_SIZE;
    while (file_size) {
        if (file_size < buff_len) {
            buff_len = file_size;
        }
        htd_memset(buff, 0, buff_len);
        app_file_read(infile_handle, buff, buff_len);
        compress_len = qlz_compress(buff, (char*)&compress_buff[sizeof(uint32_t)], buff_len, state_compress);
        file_size -= buff_len;
        
        htd_memcpy(compress_buff, &compress_len, sizeof(uint32_t));
        app_file_write(outfile_handle, compress_buff, compress_len + sizeof(uint32_t));
        totle_cmprs_size += compress_len + sizeof(uint32_t);
    }
    htd_free(compress_buff);
    htd_free(buff);
    htd_free(state_compress);
    app_file_close(outfile_handle);
    
    file_size = app_file_size(infile_handle);
    print_debug("[qlz]compressed %d bytes into %d bytes, compression ratio is %d%!", file_size, totle_cmprs_size, (totle_cmprs_size * 100) / file_size);
    app_file_close(infile_handle);
}

// @brief 解压文件
void quicklz_file_decompress(const char* in_filepath, const char* out_filepath)
{
    file_fd_t infile_handle = app_file_open(in_filepath, O_BINARY|O_RDONLY);
    if (infile_handle == NULL) {
        print_error("open file(%s) failed.", in_filepath);
        return;
    }
    file_fd_t outfile_handle = app_file_open(out_filepath, O_BINARY|O_WRONLY|O_CREAT|O_TRUNC);
    if (outfile_handle == NULL) {
        app_file_close(infile_handle);
        print_error("open file(%s) failed.", out_filepath);
        return ;
    }

    qlz_state_decompress* state_decompress = (qlz_state_decompress*) htd_malloc(sizeof(qlz_state_decompress));
    ASSERT(state_decompress);
    memset(state_decompress, 0, sizeof(qlz_state_decompress));
    uint8_t* decompress_buff = (uint8_t*)htd_malloc(DECOMPRESS_BUFFER_SIZE);
    ASSERT(decompress_buff);
    uint8_t* buff = (uint8_t*)htd_malloc(DECOMPRESS_BUFFER_SIZE + QLZ_BUFFER_PADDING);
    ASSERT(buff);

    uint32_t buff_len;
    uint32_t file_size = app_file_size(infile_handle);
    while (file_size) {
        buff_len = 0;
        app_file_read(infile_handle, &buff_len, sizeof(uint32_t));
        file_size -= sizeof(uint32_t);
        
        memset(buff, 0, COMPRESS_BUFFER_SIZE + QLZ_BUFFER_PADDING);
        memset(decompress_buff, 0, DECOMPRESS_BUFFER_SIZE);
        app_file_read(infile_handle, buff, buff_len);
        file_size -= buff_len;

        buff_len = qlz_decompress((const char *)buff, decompress_buff, state_decompress);
        app_file_write(outfile_handle, decompress_buff, buff_len);
    }

    htd_free(decompress_buff);
    htd_free(buff);
    htd_free(state_decompress);
    app_file_close(outfile_handle);
    app_file_close(infile_handle);
}
#endif


#if 0
#include "fastlz.h"
//////////////////////////////////////////////////////////////////////
#define FLZ_BUFFER_PADDING              400

// @brief 压缩文件
void fastlz_file_compress(const char* in_filepath, const char* out_filepath)
{
    file_fd_t infile_handle = app_file_open(in_filepath, O_BINARY|O_RDONLY);
    if (infile_handle == NULL) {
        print_error("open file(%s) failed.", in_filepath);
        return;
    }
    uint32_t file_size = app_file_size(infile_handle);
    
    file_fd_t outfile_handle = app_file_open(out_filepath, O_BINARY|O_WRONLY|O_CREAT|O_TRUNC);
    if (outfile_handle == NULL) {
        app_file_close(infile_handle);
        print_error("open file(%s) failed.", out_filepath);
        return ;
    }
    
    uint8_t* compress_buff = (uint8_t *)htd_malloc(COMPRESS_BUFFER_SIZE + FLZ_BUFFER_PADDING + sizeof(uint32_t));
    ASSERT(compress_buff);
    uint8_t* buff = (uint8_t *)htd_malloc(COMPRESS_BUFFER_SIZE);
    ASSERT(buff);

    uint32_t totle_cmprs_size = 0;
    uint32_t compress_len;
    uint32_t buff_len = COMPRESS_BUFFER_SIZE;
    while (file_size) {
        if (file_size < buff_len) {
            buff_len = file_size;
        }
        htd_memset(compress_buff, 0, 4);
        htd_memset(buff, 0, buff_len);
        app_file_read(infile_handle, buff, buff_len);
        compress_len = fastlz_compress_level(1, buff, buff_len, &compress_buff[sizeof(uint32_t)]);
        file_size -= buff_len;
        ASSERT(compress_len <= COMPRESS_BUFFER_SIZE + FLZ_BUFFER_PADDING);
        
        htd_memcpy(compress_buff, &compress_len, sizeof(uint32_t));
        app_file_write(outfile_handle, compress_buff, compress_len + sizeof(uint32_t));
        totle_cmprs_size += compress_len + sizeof(uint32_t);
    }
    htd_free(compress_buff);
    htd_free(buff);
    app_file_close(outfile_handle);
    
    file_size = app_file_size(infile_handle);
    print_debug("[flz]compressed %d bytes into %d bytes, compression ratio is %d%!", file_size, totle_cmprs_size, (totle_cmprs_size * 100) / file_size);
    app_file_close(infile_handle);
}

// @brief 解压文件
void fastlz_file_decompress(const char* in_filepath, const char* out_filepath)
{
    file_fd_t infile_handle = app_file_open(in_filepath, O_BINARY|O_RDONLY);
    if (infile_handle == NULL) {
        print_error("open file(%s) failed.", in_filepath);
        return;
    }
    file_fd_t outfile_handle = app_file_open(out_filepath, O_BINARY|O_WRONLY|O_CREAT|O_TRUNC);
    if (outfile_handle == NULL) {
        app_file_close(infile_handle);
        print_error("open file(%s) failed.", out_filepath);
        return ;
    }

    uint8_t* decompress_buff = (uint8_t*)htd_malloc(DECOMPRESS_BUFFER_SIZE);
    ASSERT(decompress_buff);
    uint8_t* buff = (uint8_t*)htd_malloc(DECOMPRESS_BUFFER_SIZE + FLZ_BUFFER_PADDING);
    ASSERT(buff);

    uint32_t buff_len;
    uint32_t file_size = app_file_size(infile_handle);
    while (file_size) {
        app_file_read(infile_handle, &buff_len, sizeof(uint32_t));
        file_size -= sizeof(uint32_t);
        
        memset(buff, 0, COMPRESS_BUFFER_SIZE + FLZ_BUFFER_PADDING);
        memset(decompress_buff, 0, DECOMPRESS_BUFFER_SIZE);
        app_file_read(infile_handle, buff, buff_len);
        file_size -= buff_len;

        buff_len = fastlz_decompress(buff, buff_len, decompress_buff, DECOMPRESS_BUFFER_SIZE);
        app_file_write(outfile_handle, decompress_buff, buff_len);
    }
    
    htd_free(decompress_buff);
    htd_free(buff);
    app_file_close(outfile_handle);
    app_file_close(infile_handle);
}

#endif /**/

// @brief 应用程序启动
int main(int argc, char** argv)
{
    app_board_init();

#if 0
    if (argc != 3) {
        print_error("Usage: %s [in file] [out file]", argv[0]);
        return 0;
    }
    print_debug("in file: %s", argv[1]);
    print_debug("out file: %s", argv[2]);
    //quicklz_file_compress(argv[1], argv[2]);
    //quicklz_file_decompress(argv[1], argv[2]);
    //fastlz_file_compress(argv[1], argv[2]);
    //fastlz_file_decompress(argv[1], argv[2]);
#endif

    htd_task_sch_loop(NULL);
    return 0;
}

