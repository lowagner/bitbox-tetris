#include "bitbox.h"
#include "chiptune.h"
#include "common.h"
#include "name.h"
#include "io.h"

#include "fatfs/ff.h"
#include <string.h> // strlen
#include <stdlib.h> // qsort

int io_mounted = 0;
FATFS fat_fs;
FIL fat_file;
FRESULT fat_result;
char old_base_filename[9] CCM_MEMORY;
uint8_t old_chip_volume CCM_MEMORY;

char available_filenames[255][9] CCM_MEMORY;
uint8_t available_index CCM_MEMORY;
uint8_t available_count CCM_MEMORY;

void io_message_from_error(uint8_t *msg, FileError error, int save_not_load)
{
    switch (error)
    {
    case NoError:
        if (save_not_load == 1)
            strcpy((char *)msg, "saved!");
        else
            strcpy((char *)msg, "loaded!");
        break;
    case MountError:
        strcpy((char *)msg, "fs unmounted!");
        break;
    case ConstraintError:
        strcpy((char *)msg, "unconstrained!");
        break;
    case OpenError:
        strcpy((char *)msg, "no open!");
        break;
    case ReadError:
        strcpy((char *)msg, "no read!");
        break;
    case WriteError:
        strcpy((char *)msg, "no write!");
        break;
    case NoDataError:
        strcpy((char *)msg, "no data!");
        break;
    case MissingDataError:
        strcpy((char *)msg, "miss data!");
        break;
    case BotchedIt:
        strcpy((char *)msg, "fully bungled.");
        break;
    }
}

FileError io_init()
{
    available_count = 0;
    available_index = 255;
    available_filenames[0][0] = 0;

    old_base_filename[0] = 0;
    fat_result = f_mount(&fat_fs, "", 1); // mount now...
    if (fat_result != FR_OK)
    {
        io_mounted = 0;
        return MountError;
    }
    io_mounted = 1;

    return NoError;
}

FileError io_set_extension(char *filename, const char *ext)
{
    if (io_mounted == 0)
    {
        if (io_init())
            return MountError;
    }
    io_set_recent_filename();

    int k = 0;
    for (; k<8 && base_filename[k]; ++k)
    { 
        filename[k] = base_filename[k];
    }
    --k;
    filename[++k] = '.';
    filename[++k] = *ext++;
    filename[++k] = *ext++;
    filename[++k] = *ext;
    filename[++k] = 0;
    return NoError;
}

FileError io_open_or_zero_file(const char *fname, unsigned int size)
{
    if (size == 0)
        return NoDataError;

    fat_result = f_open(&fat_file, fname, FA_WRITE | FA_OPEN_EXISTING);
    if (fat_result != FR_OK)
    {
        fat_result = f_open(&fat_file, fname, FA_WRITE | FA_OPEN_ALWAYS);
        if (fat_result != FR_OK)
            return OpenError;
        uint8_t zero[128] = {0};
        while (size) 
        {
            int write_size;
            if (size <= 128)
            {
                write_size = size;
                size = 0;
            }
            else
            {
                write_size = 128;
                size -= 128;
            }
            UINT bytes_get;
            f_write(&fat_file, zero, write_size, &bytes_get);
            if (bytes_get != write_size)
            {
                f_close(&fat_file);
                return MissingDataError;
            }
        }
    }
    return NoError;
}

FileError io_set_recent_filename()
{
    int filename_len = strlen(base_filename);
    if (filename_len == 0)
        return ConstraintError;

    if (io_mounted == 0)
    {
        if (io_init())
            return MountError;
    }
    
    if (strcmp(old_base_filename, base_filename) == 0 && chip_volume == old_chip_volume)
        return NoError; // don't rewrite  

    fat_result = f_open(&fat_file, "RECENT16.TXT", FA_WRITE | FA_CREATE_ALWAYS); 
    if (fat_result != FR_OK) 
        return OpenError;
    UINT bytes_get;
    fat_result = f_write(&fat_file, base_filename, filename_len, &bytes_get);
    if (fat_result != FR_OK)
    {
        f_close(&fat_file);
        return WriteError;
    }
    if (bytes_get != filename_len)
    {
        f_close(&fat_file);
        return MissingDataError;
    }
    strcpy(old_base_filename, base_filename);
    uint8_t msg[2] = { '\n', chip_volume };
    fat_result = f_write(&fat_file, msg, 2, &bytes_get);
    f_close(&fat_file);
    if (fat_result != FR_OK)
        return WriteError;
    if (bytes_get != 2)
        return MissingDataError;
    old_chip_volume = chip_volume;
    return NoError;
}

#ifdef EMULATOR
#define ROOT_DIR "."
#else
#define ROOT_DIR ""
#endif
static int cmp(const void *p1, const void *p2){
    return strcmp( (char * const ) p1, (char * const ) p2);
}
void io_list_music() // adapted from list_roms from bitbox/2nd_boot
{
    available_count = 0;
    available_index = 255;
    available_filenames[0][0] = 0;

    message("listing music\n");
    
    FRESULT res;
    FILINFO fno;
    DIR dir;

    char *fn;   /* This function is assuming non-Unicode cfg. */

    if (f_opendir(&dir, ROOT_DIR) != FR_OK) 
    {
        message("bad dir\n");
        return;
    }
    for (available_count=0;;) 
    {
        res = f_readdir(&dir, &fno);                   /* Read a directory item */
        fn = fno.fname;
        if (res != FR_OK || fn[0] == 0) break;  /* Break on error or end of dir */
        /* Ignore dot entry and dirs */
        if (fn[0] == '.') continue;
        message("check: %s\n", fn);
        if (fno.fattrib & AM_DIR) 
        {
            message(" was a directory\n");
            continue;
        }
        if (strstr(fn,".A16"))
        {
            message(" got A16.\n");
            char *src = fn;
            char *dst = available_filenames[available_count];
            int found_period = 0;
            for (int i=0; i<8; ++i)
            {
                *dst = *src++;
                if (*dst == '.')
                {
                    found_period = 1;
                    break;
                }
                ++dst;
            }
            *dst = 0;
            if (!found_period)
            {
                message("weird, expected period in filename, skipping %s\n", fn);
                continue;
            }
            if (++available_count == 255)
                break;
        }
    }
    f_closedir(&dir);
    message("got to end with count %d\n", available_count);
    // sort it
    qsort(available_filenames, available_count, 9, cmp);
    for (int i=0; i<available_count; ++i)
    {
        message("got filename %s\n", available_filenames[i]);
    }
}

FileError io_get_recent_filename()
{
    if (io_mounted == 0)
    {
        if (io_init())
            return MountError;
    }

    io_list_music();

    fat_result = f_open(&fat_file, "RECENT16.TXT", FA_READ | FA_OPEN_EXISTING); 
    if (fat_result != FR_OK) 
        return OpenError;

    UINT bytes_get;
    uint8_t buffer[10];
    fat_result = f_read(&fat_file, &buffer[0], 10, &bytes_get); 
    f_close(&fat_file);
    if (fat_result != FR_OK)
        return ReadError;
    if (bytes_get == 0)
        return NoDataError;
    int i=0;
    while (i < bytes_get)
    {
        if (buffer[i] == '\n')
        {
            base_filename[i] = 0;
            if (++i < bytes_get)
                chip_volume = buffer[i];
            goto search_for_filename;
        }
        if (i < 8)
            base_filename[i] = buffer[i];
        else
        {
            base_filename[8] = 0;
            message("got weird value in RECENT16.txt: %s+%c\n", base_filename, buffer[i]);
            goto search_for_filename;
        }
        ++i;
    }
    base_filename[i] = 0;

    search_for_filename:
    if (available_count == 0)
    {
        available_index = 255;
        return ConstraintError;
    }
    char *find = bsearch(base_filename, available_filenames, available_count, 9, cmp);
    if (!find)
    {
        available_index = 0;
        strcpy(base_filename, available_filenames[0]);
        return ConstraintError;
    }
   
    message("got binary search delta %d,", (int)(find-available_filenames[0]));
    available_index = (int)(find-available_filenames[0])/9;
    message(" dividing by nine to get %d\n", available_index);
    if (available_index == 255 || available_index >= available_count || strcmp(base_filename, find) != 0)
    {
        message("that was unexpected... searched for %s but got %s\n", base_filename, find);
        available_index = 0;
        strcpy(base_filename, available_filenames[0]);
        return ConstraintError;
    }
    return NoError;
}

FileError io_load_instrument(unsigned int i)
{
    char filename[13];
    if (io_set_extension(filename, "I16"))
        return MountError;
    
    fat_result = f_open(&fat_file, filename, FA_READ | FA_OPEN_EXISTING);
    if (fat_result != FR_OK)
        return OpenError;

    UINT bytes_get; 
    if (i >= 16)
    {
        for (i=0; i<16; ++i)
        {
            uint8_t read;
            fat_result = f_read(&fat_file, &read, 1, &bytes_get);
            if (fat_result != FR_OK)
            {
                f_close(&fat_file);
                return ReadError;
            }
            if (bytes_get != 1)
            {
                f_close(&fat_file);
                return MissingDataError;
            }
            instrument[i].is_drum = read&15;
            instrument[i].octave = read >> 4;
            fat_result = f_read(&fat_file, &instrument[i].cmd[0], MAX_INSTRUMENT_LENGTH, &bytes_get);
            if (fat_result != FR_OK)
            {
                f_close(&fat_file);
                return ReadError;
            }
            if (bytes_get != MAX_INSTRUMENT_LENGTH)
            {
                f_close(&fat_file);
                return MissingDataError;
            }
        }
        f_close(&fat_file);
        return NoError;
    }

    f_lseek(&fat_file, i*(MAX_INSTRUMENT_LENGTH+1)); 
    uint8_t read;
    fat_result = f_read(&fat_file, &read, 1, &bytes_get);
    if (fat_result != FR_OK)
    {
        f_close(&fat_file);
        return ReadError;
    }
    if (bytes_get != 1)
    {
        f_close(&fat_file);
        return MissingDataError;
    }
    instrument[i].is_drum = read&15;
    instrument[i].octave = read >> 4;
    fat_result = f_read(&fat_file, &instrument[i].cmd[0], MAX_INSTRUMENT_LENGTH, &bytes_get);
    if (fat_result != FR_OK)
    {
        f_close(&fat_file);
        return ReadError;
    }
    if (bytes_get != MAX_INSTRUMENT_LENGTH)
    {
        f_close(&fat_file);
        return MissingDataError;
    }
    
    f_close(&fat_file);
    return NoError;
}

FileError io_save_instrument(unsigned int i)
{
    char filename[13];
    if (io_set_extension(filename, "I16"))
        return MountError; 

    if (i >= 16)
    {
        fat_result = f_open(&fat_file, filename, FA_WRITE | FA_OPEN_ALWAYS);
        if (fat_result != FR_OK)
            return OpenError;

        for (i=0; i<16; ++i)
        {
            UINT bytes_get; 
            uint8_t write = (instrument[i].is_drum ? 1 : 0) | (instrument[i].octave << 4);
            fat_result = f_write(&fat_file, &write, 1, &bytes_get);
            if (fat_result != FR_OK)
            {
                f_close(&fat_file);
                return WriteError;
            }
            if (bytes_get != 1)
            {
                f_close(&fat_file);
                return MissingDataError;
            }
            fat_result = f_write(&fat_file, &instrument[i].cmd[0], MAX_INSTRUMENT_LENGTH, &bytes_get);
            if (fat_result != FR_OK)
            {
                f_close(&fat_file);
                return WriteError;
            }
            if (bytes_get != MAX_INSTRUMENT_LENGTH)
            {
                f_close(&fat_file);
                return MissingDataError;
            }
        }
        f_close(&fat_file);
        return NoError;
    }

    FileError ferr = io_open_or_zero_file(filename, 16*(MAX_INSTRUMENT_LENGTH+1));
    if (ferr)
        return ferr;

    f_lseek(&fat_file, i*(MAX_INSTRUMENT_LENGTH+1)); 
    UINT bytes_get; 
    uint8_t write = (instrument[i].is_drum ? 1 : 0) | (instrument[i].octave << 4);
    fat_result = f_write(&fat_file, &write, 1, &bytes_get);
    if (fat_result != FR_OK)
    {
        f_close(&fat_file);
        return WriteError;
    }
    if (bytes_get != 1)
    {
        f_close(&fat_file);
        return MissingDataError;
    }
    fat_result = f_write(&fat_file, &instrument[i].cmd[0], MAX_INSTRUMENT_LENGTH, &bytes_get);
    if (fat_result != FR_OK)
    {
        f_close(&fat_file);
        return WriteError;
    }
    if (bytes_get != MAX_INSTRUMENT_LENGTH)
    {
        f_close(&fat_file);
        return MissingDataError;
    }
    
    f_close(&fat_file);
    return NoError;
}

FileError io_load_verse(unsigned int i)
{
    char filename[13];
    if (io_set_extension(filename, "V16"))
        return MountError; 

    fat_result = f_open(&fat_file, filename, FA_READ | FA_OPEN_EXISTING);
    if (fat_result != FR_OK)
        return OpenError;

    if (i >= 16)
    {
        for (i=0; i<16; ++i)
        {
            UINT bytes_get; 
            fat_result = f_read(&fat_file, &chip_track[i], sizeof(chip_track[0]), &bytes_get);
            if (fat_result != FR_OK)
            {
                f_close(&fat_file);
                return ReadError;
            }
            if (bytes_get != sizeof(chip_track[0]))
            {
                f_close(&fat_file);
                return MissingDataError;
            }
        }
        f_close(&fat_file);
        return NoError;
    }

    f_lseek(&fat_file, i*sizeof(chip_track[0])); 
    UINT bytes_get; 
    fat_result = f_read(&fat_file, &chip_track[i], sizeof(chip_track[0]), &bytes_get);
    if (fat_result != FR_OK)
    {
        f_close(&fat_file);
        return ReadError;
    }
    if (bytes_get != sizeof(chip_track[0]))
    {
        f_close(&fat_file);
        return MissingDataError;
    }
    f_close(&fat_file);
    return NoError;
}

FileError io_save_verse(unsigned int i)
{
    char filename[13];
    if (io_set_extension(filename, "V16"))
        return MountError; 

    if (i >= 16)
    {
        fat_result = f_open(&fat_file, filename, FA_WRITE | FA_OPEN_ALWAYS);
        if (fat_result != FR_OK)
            return OpenError;

        for (i=0; i<16; ++i)
        {
            UINT bytes_get; 
            fat_result = f_write(&fat_file, &chip_track[i], sizeof(chip_track[0]), &bytes_get);
            if (fat_result != FR_OK)
            {
                f_close(&fat_file);
                return WriteError;
            }
            if (bytes_get != sizeof(chip_track[0]))
            {
                f_close(&fat_file);
                return MissingDataError;
            }
        }
        f_close(&fat_file);
        return NoError;
    }

    FileError ferr = io_open_or_zero_file(filename, 16*sizeof(chip_track[0]));
    if (ferr)
        return ferr;

    f_lseek(&fat_file, i*sizeof(chip_track[0])); 
    UINT bytes_get; 
    fat_result = f_write(&fat_file, &chip_track[i], sizeof(chip_track[0]), &bytes_get);
    if (fat_result != FR_OK)
    {
        f_close(&fat_file);
        return WriteError;
    }
    if (bytes_get != sizeof(chip_track[0]))
    {
        f_close(&fat_file);
        return MissingDataError;
    }
    f_close(&fat_file);
    return NoError;
}

FileError io_load_anthem()
{
    char filename[13];
    if (io_set_extension(filename, "A16"))
        return MountError; 

    fat_result = f_open(&fat_file, filename, FA_READ | FA_OPEN_EXISTING); 
    if (fat_result)
        return OpenError;

    // set some defaults
    track_length = 16;
    song_speed = 4;

    UINT bytes_get; 
    fat_result = f_read(&fat_file, &song_length, 1, &bytes_get);
    if (fat_result != FR_OK)
    {
        f_close(&fat_file);
        return ReadError;
    }
    if (bytes_get != 1)
    {
        f_close(&fat_file);
        return MissingDataError;
    }

    if (song_length < 16 || song_length > MAX_SONG_LENGTH)
    {
        message("got song length %d\n", song_length);
        song_length = 16;
        f_close(&fat_file);
        return ConstraintError;
    }
    
    fat_result = f_read(&fat_file, &chip_song[0], 2*song_length, &bytes_get);
    if (fat_result != FR_OK)
    {
        f_close(&fat_file);
        return ReadError;
    }
    if (bytes_get != 2*song_length)
    {
        f_close(&fat_file);
        return MissingDataError;
    }

    f_close(&fat_file);
    return NoError;
}

FileError io_save_anthem()
{
    char filename[13];
    if (io_set_extension(filename, "A16"))
        return MountError; 

    fat_result = f_open(&fat_file, filename, FA_WRITE | FA_CREATE_ALWAYS); 
    if (fat_result)
        return OpenError;

    UINT bytes_get; 
    fat_result = f_write(&fat_file, &song_length, 1, &bytes_get);
    if (fat_result != FR_OK)
    {
        f_close(&fat_file);
        return WriteError;
    }
    if (bytes_get != 1)
    {
        f_close(&fat_file);
        return MissingDataError;
    }
    
    fat_result = f_write(&fat_file, &chip_song[0], 2*song_length, &bytes_get);
    if (fat_result != FR_OK)
    {
        f_close(&fat_file);
        return WriteError;
    }
    if (bytes_get != 2*song_length)
    {
        f_close(&fat_file);
        return MissingDataError;
    }

    f_close(&fat_file);
    return NoError;
}
