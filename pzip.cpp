/****************************************************************************
 *  This file is part of PZip project                                       *
 *  Contents: main routine                                                  *
 *  Comments: system & compiler dependent file                              *
 *  CHANGELOG:
 *  Initial algorithm and code: Dmitry Shkarin 1997, 1999-2001              *
 *  Ed Wildgoose (2004): Converted to "pzip"                                * 
 *  Copyright 1999 by Dmitry Subbotin, 2004 Ed Wildgoose                    *
 *                                                                         *
 *   Permission is hereby granted, free of charge, to any person obtaining *
 *   a copy of this software and associated documentation files (the       *
 *   "Software"), to deal in the Software without restriction, including   *
 *   without limitation the rights to use, copy, modify, merge, publish,   *
 *   distribute, sublicense, and/or sell copies of the Software, and to    *
 *   permit persons to whom the Software is furnished to do so, subject to *
 *   the following conditions:                                             *
 *                                                                         *
 *   The above copyright notice and this permission notice shall be        *
 *   included in all copies or substantial portions of the Software.       *
 *                                                                         *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    *
 *   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*
 *   IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR     *
 *   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, *
 *   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR *
 *   OTHER DEALINGS IN THE SOFTWARE.                                       *
 ***************************************************************************/
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "pz_type.hpp"
#include "pz_interface.hpp"


#define BACKSLASH '\\'
#define USAGE_STR "Usage: pzip [switches] < - | FileName[s] | Wildcard[s]>\n"
static const char* pFName;
static struct FILE_INFO { // FileLength & CRC? Hmm, maybe later...
    DWORD attrib;
    WORD  time,date;
} _PACK_ATTR ai;

// OS Specific code
#if defined(_WIN32_ENVIRONMENT_)
    #include <conio.h>
    #include "XGetopt.h"

    inline void EnvSetNormAttr(const char* FName) { SetFileAttributes(FName,FILE_ATTRIBUTE_NORMAL); }
    inline int                         EnvGetCh() { return getch(); }
    inline void           EnvGetCWD(char* CurDir) { GetCurrentDirectory(260,CurDir); }
    inline void EnvSetDateTimeAttr(const char* WrkStr)
    {
        FILETIME ft;
        DosDateTimeToFileTime(ai.date,ai.time,&ft);
        HANDLE hndl=CreateFile(WrkStr,GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,0);
        SetFileTime(hndl,&ft,NULL,&ft);         
        CloseHandle(hndl);
        SetFileAttributes(WrkStr,ai.attrib);
    }
    struct ENV_FIND_RESULT: public WIN32_FIND_DATA {
        const char*  getFName() const { return cFileName; }
        void copyDateTimeAttr() const {
            ai.attrib=dwFileAttributes;
            FileTimeToDosDateTime(&ftLastWriteTime,&ai.date,&ai.time);
        }
    };

    struct ENV_FILE_FINDER {
        HANDLE hndl;
        ENV_FIND_RESULT Rslt;
        ENV_FIND_RESULT getResult() { return Rslt; }
        BOOL isFileValid() {
            return ((Rslt.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0);
        }
        BOOL findFirst(const char* Pattern) {
            return ((hndl=FindFirstFile(Pattern,&Rslt)) != INVALID_HANDLE_VALUE);
        }
        BOOL findNext() { return FindNextFile(hndl,&Rslt); }
        void findStop() { FindClose(hndl); }
    };

#elif defined(_DOS32_ENVIRONMENT_)
    #include <conio.h>
    #include <dos.h>
    #include <direct.h>
    #include "XGetopt.h"
    #if defined(__DJGPP__)
    #include <unistd.h>
    #include <crt0.h>
    #undef BACKSLASH
    #define BACKSLASH '/'
    char **__crt0_glob_function (char *arg) { return 0; }
    void   __crt0_load_environment_file (char *progname) { }
    #endif /* defined(__DJGPP__) */

    inline void EnvSetNormAttr(const char* FName) { _dos_setfileattr(FName,_A_NORMAL); }
    inline int                         EnvGetCh() { return getch(); }
    inline void           EnvGetCWD(char* CurDir) { getcwd(CurDir,260); }
    inline void EnvSetDateTimeAttr(const char* WrkStr)
    {
        FILE* fpOut = fopen(WrkStr,"a+b");
        _dos_setftime(fileno(fpOut),ai.date,ai.time);
        fclose(fpOut);
        _dos_setfileattr(WrkStr,ai.attrib);
    }
    struct ENV_FIND_RESULT: public find_t {
        const char*  getFName() const { return name; }
        void copyDateTimeAttr() const {
            ai.attrib=attrib;
            ai.time=wr_time;                    ai.date=wr_date;
        }
    };
    struct ENV_FILE_FINDER {
        ENV_FIND_RESULT Rslt;
        ENV_FIND_RESULT getResult() { return Rslt; }
        BOOL isFileValid() { return PPMD_TRUE; }
        BOOL findFirst(const char* Pattern) {
            return (_dos_findfirst(Pattern,_A_RDONLY | _A_HIDDEN | _A_SYSTEM | _A_ARCH,&Rslt) == 0);
        }
        BOOL findNext() { return (_dos_findnext(&Rslt) == 0); }
        void findStop() { }
    };

#elif defined(_POSIX_ENVIRONMENT_)
    #define _POSIX_SOURCE
    #undef BACKSLASH
    #define BACKSLASH '/'
    #include <sys/stat.h>
    #include <time.h>
    #include <utime.h>
    #include <dirent.h>
    #include <unistd.h>
    #include <fnmatch.h>
    #if defined(__DJGPP__)
    #include <crt0.h>
    char **__crt0_glob_function (char *arg) { return 0; }
    void   __crt0_load_environment_file (char *progname) { }
    #endif /* defined(__DJGPP__) */

    inline int stricmp(const char *string1,const char *string2 ) {return strcasecmp(string1, string2);};
    inline void EnvSetNormAttr(const char* FName) { chmod(FName,S_IWUSR); }
    inline int                         EnvGetCh() { return getchar(); }
    inline void           EnvGetCWD(char* CurDir) { getcwd(CurDir,260); }
    inline void EnvSetDateTimeAttr(const char* WrkStr)
    {
        struct utimbuf t;
        t.actime=t.modtime=(ai.date << 16)+ai.time;
        utime(WrkStr,&t);                       
        chmod(WrkStr,ai.attrib);
    }
    struct ENV_FIND_RESULT {
        dirent de;
        struct stat st;
        const char*  getFName() const { return de.d_name; }
        void copyDateTimeAttr() const {
            ai.attrib=st.st_mode;
            ai.time=st.st_mtime & 0xFFFF;       
            ai.date=st.st_mtime >> 16;
        }
    };
    struct ENV_FILE_FINDER {
        const char* pPattern;
        DIR* dir;
        dirent* de;
        struct stat st;
        ENV_FIND_RESULT getResult() {
            ENV_FIND_RESULT Rslt;
            Rslt.de=*de;                        
            Rslt.st=st;
            return Rslt;
        }
        BOOL isFileValid() {
            return (fnmatch(pPattern,de->d_name,FNM_NOESCAPE) == 0 &&
                    stat(de->d_name,&st) == 0 && (st.st_mode & S_IRUSR) != 0 &&
                    st.st_nlink == 1);
        }
        BOOL findFirst(const char* Pattern) {
            pPattern=Pattern;
            return ((dir=opendir(".")) && (de=readdir(dir)) != NULL);
        }
        BOOL findNext() { return ((de=readdir(dir)) != NULL); }
        void findStop() { closedir(dir); }
    };

#else /* _UNKNOWN_ENVIRONMENT_ */
    #pragma message ("unknown environment:")
    #pragma message ("    1. _fastcall and _stdcall keywords are disabled")
    #pragma message ("    2. wildcards and file attributes are disabled")

    #undef  USAGE_STR
    #define USAGE_STR "Usage: PPMd [switches] < - | FileName[s] >\n"
    inline void     EnvSetNormAttr(const char* ) {}
    inline int                        EnvGetCh() { return getchar(); }
    inline void          EnvGetCWD(char* CurDir) { CurDir[0]=0; }
    inline void EnvSetDateTimeAttr(const char* ) {}
    struct ENV_FIND_RESULT {
        char FName[260];
        const char*  getFName() const { return FName; }
        void copyDateTimeAttr() const {}
    };
    struct ENV_FILE_FINDER {
        const char* pPattern;
        ENV_FIND_RESULT getResult() {
            ENV_FIND_RESULT Rslt;               strcpy(Rslt.FName,pPattern);
            return Rslt;
        }
        BOOL isFileValid() { return PPMD_TRUE; }
        BOOL findFirst(const char* Pattern) {
            pPattern=Pattern;                   return PPMD_TRUE;
        }
        BOOL         findNext() { return PPMD_FALSE; }
        void findStop() {}
    };
#endif /* defined(__WIN32_ENVIRONMENT) */

// Structure to hold our list of files found from scanning the args line
struct FILE_LIST_NODE {
    FILE_LIST_NODE* next;
    ENV_FIND_RESULT efr;
    FILE_LIST_NODE(const ENV_FIND_RESULT& Data,FILE_LIST_NODE** PrevNode) {
        efr=Data;                           next=*PrevNode;
        *PrevNode=this;
    }
    void destroy(FILE_LIST_NODE** PrevNode) {
        *PrevNode=next;                     delete this;
    }
};

static const char* const MTxt[] = { "Can`t open file %s\n",
    "read/write error for files %s/%s\n", 
    "Out of memory!\n",
    "User break\n", 
    "unknown command: %s\n", 
    "unknown switch: %s %s\n",
    "designed and written by:\n"
    "    Dmitry Shkarin <dmitry.shkarin@mtu-net.ru>\n"
    "    and Ed Wildgoose <pzip@wildgooses.com>\n\n"
    USAGE_STR
    "\n"
    "   -h --help           print this message\n"
    "   -d --decompress     force decompression\n"
    "   -z --compress       force compression\n"
    "   -k --keep           keep (don't delete) input files\n"
    "   -f --force          overwrite existing output files\n"
    "   -t --test           test compressed file integrity\n"
    "   -c --stdout         output to standard out\n"
    "   -q --quiet          suppress noncritical error messages\n"
    "   -v --verbose        be verbose\n"
    "   -L --license        display software version & license\n"
    "   -V --version        display software version & license\n"
    "   -1 .. -9            set -m/-o/-r to decent defaults\n"
    "\n\n"
    "   Switches (for encoding only):\n"
    "   -f Name - set output file name to Name\n"
    "   -m N    - use N MB memory - [1,256], default: %d\n"
    "   -o N    - set model order to N - [2,%d], default: %d\n"
    "   -r N    - set method of model restoration at memory insufficiency:\n"
    "   \t-r 0 - restart model from scratch (default)\n"
    "   \t-r 1 - cut off model (slow)\n"
    "   \t-r 2 - freeze model (dangerous)\n"
    "\n"
    "   If invoked as `pzip', default action is to compress.\n"
    "          as `punzip',  default action is to decompress.\n"
    "          as `pzcat', default action is to decompress to stdout.\n"
    "\n"
    "   If no file names are given, pzip compresses or decompresses\n"
    "   from standard input to standard output.\n",
    "PZip. Fast PPMII compressor, variant %c, version %s\n",
    "Can't guess original name for %s, using %s\n",
    "Output file %s already exists\n",
    "Can't delete existing output file: %s\n"
};


template <class T>
inline T CLAMP(const T& X,const T& LoX,const T& HiX) { return (X >= LoX)?((X <= HiX)?(X):(HiX)):(LoX); }
template <class T>
inline void SWAP(T& t1,T& t2) { T tmp=t1; t1=t2; t2=tmp; }


static char* _STDCALL ChangeExtRare(const char* In,char* Out,const char* Ext)
{
    char* RetVal=Out;
    const char* p=strrchr(In,'.');
    if (!p || strrchr(In,BACKSLASH) > p)    p=In+strlen(In);
    do { *Out++ = *In++; } while (In != p);
    *Out++='.';
    while((*Out++ = *Ext++) != 0)           ;
    return RetVal;
}

inline BOOL RemoveFile(const char* FName)
{
    EnvSetNormAttr(FName);                  
    return (remove(FName) == 0);
}

static BOOL _STDCALL TestExists(const char* FName)
{
    FILE* fp=fopen(FName,"rb");
    if ( !fp )                              
        return PPMD_FALSE;
    fclose(fp);
    return PPMD_TRUE;
}

static BOOL _STDCALL TestAccessRare(const char* FName)
{
static BOOL YesToAll=PPMD_FALSE;
    FILE* fp=fopen(FName,"rb");
    if ( !fp )                              
        return PPMD_TRUE;
    fclose(fp);
    if ( YesToAll )                         
        return RemoveFile(FName);
    fprintf(stderr, "%s already exists, overwrite?: <Y>es, <N>o, <A>ll, <Q>uit?",FName);
    int ch = toupper(EnvGetCh());
    putchar('\n');

    for ( ; ; )
        switch ( ch ) {
            case 'A':                       YesToAll=PPMD_TRUE;
            case '\r': case 'Y':            return RemoveFile(FName);
            case 0x1B: case 'Q':            fprintf(stderr, MTxt[3]); exit(-1);
            case 'N':                       return PPMD_FALSE;
        }
}

inline FILE* FOpen(const char* FName,const char* mode)
{
    FILE* fp=fopen(FName,mode);
    if ( !fp ) { fprintf(stderr, MTxt[0],FName);     exit(-1); }
    setvbuf(fp,NULL,_IOFBF,64*1024);        
    return fp;
}

BOOL CopyFileAttr(const char *Src, const char *Dest)
{
    ENV_FILE_FINDER eff;
    if ( eff.findFirst(Src) ) {
        if ( eff.isFileValid() ) {
            eff.getResult().copyDateTimeAttr();
            EnvSetDateTimeAttr(Dest);
        }
        eff.findStop();
    }
    return 0;
}

inline BOOL EncodeFile(const char* OrigFile, const char* ArcFile, 
                       int MaxOrder,int SASize,MR_METHOD MRMethod, 
                       BOOL Overwrite = PPMD_FALSE, BOOL Verbose = PPMD_FALSE)
{
    FILE *fpIn, *fpOut;
    if (strcmp(OrigFile,"-")==0)
        fpIn = stdin;
    else
        fpIn = FOpen(OrigFile,"rb");
    if (strcmp(ArcFile,"-")==0) 
        fpOut = stdout;
    else {
        if (TestExists(ArcFile)) {
            if (!Overwrite) {
                fprintf(stderr, MTxt[9], ArcFile);
                return 0;
            } else {
                if (!RemoveFile(ArcFile)) {
                    fprintf(stderr, MTxt[10], ArcFile);
                    return 0;
                }
            }
        }
        fpOut = FOpen(ArcFile,"a+b");
    }

    PZA_Encoder myEncoder;
    myEncoder.SetParams(SASize, MaxOrder, MRMethod, 0, NULL);
    myEncoder.SetVerbose(Verbose);
    
    PZ_Out_File OutFile(fpOut);
    PZ_In_File InFile(fpIn);

    PZ_RESULT rtn = myEncoder.Encode(&InFile,&OutFile);
    rtn = myEncoder.Flush(&OutFile,&InFile);

    putchar('\n');
    if (rtn != P_OK) {
        fprintf(stderr, MTxt[1],OrigFile,ArcFile);
        exit(-1);
    }
    fclose(fpIn);                           
    fclose(fpOut);

    if (strcmp(OrigFile, "-") && strcmp(ArcFile, "-"))
        CopyFileAttr(OrigFile, ArcFile);

    return 1;
}

inline BOOL DecodeFile(const char* ArcFile, const char* DecodedFile, 
                       BOOL Overwrite = PPMD_FALSE, BOOL Verbose = PPMD_FALSE)
{

    FILE *fpIn, *fpOut;
    // Open output
    if (strcmp(DecodedFile,"-")==0)
        fpOut = stdout;
    else {
        if ( TestExists(DecodedFile) ) {
            if ( !Overwrite ) {
                fprintf(stderr, MTxt[1],DecodedFile,DecodedFile);      
	            return 0;
            } else {
                if (!RemoveFile(DecodedFile)) {
                    fprintf(stderr, MTxt[10], ArcFile);
                    return 0;
                }
            }
        }
        fpOut = FOpen(DecodedFile,"wb");
    }
    // Open input
    if (strcmp(ArcFile,"-")==0) 
        fpIn = stdin;
    else
        fpIn = FOpen(ArcFile,"rb");

    PZA_Decoder myDecoder;
    myDecoder.SetVerbose(Verbose);

    PZ_Out_File OutFile(fpOut);
    PZ_In_File InFile(fpIn);

    PZ_RESULT rtn = myDecoder.Decode(&InFile,&OutFile);

    if (Verbose) putchar('\n');
    if (ferror(fpOut) || ferror(fpIn) || feof(fpIn)) {
        fprintf(stderr, MTxt[1],ArcFile,DecodedFile);      
        exit(-1);
    }
    fclose(fpOut);                          
    fclose(fpIn);

    // Copy date and time unless we are using stdin/out
    if (strcmp(DecodedFile, "-") && strcmp(ArcFile, "-"))
        CopyFileAttr(ArcFile, DecodedFile);

    return 1;
}



/* Change the extension on the filename as follows:
*    If encoding then add ".pz"
*    If decoding then remove ".pz"
*    ...unless the extension isn't .pz in which case append ".out"
*    Returns true unless it had to put .out on the end */
BOOL ChangeExt(const char *Orig, char *New, BOOL encode)
{
    if (encode) {
        strcpy(New, Orig);
        strcat(New, ".pz");
        return PPMD_TRUE;
    } else {
        strcpy(New, Orig);
        if ((strlen(Orig) >= 4) && (stricmp(Orig+strlen(Orig)-3, ".pz")==0) ) {
            New[strlen(Orig)-3] = 0; //truncate the string to exclude the .pz extension
            return PPMD_TRUE;
        } else {
            // Not a recognizable extension.  Slap .out on the end
            strcat(New, ".out");
            return PPMD_FALSE;
        }
    }
}
            
int main(int argc, char *argv[])
{

    BOOL DeleteFile=PPMD_TRUE;
    BOOL Overwrite=PPMD_FALSE;
    BOOL EncodeFlag=PPMD_TRUE;
    BOOL Verbose=PPMD_FALSE;
    BOOL WriteStdout=PPMD_FALSE;
    int i, MaxOrder=-1, SASize=-1;
    int MRMethod=-1;


    if (argc==1) {
        // No args given
        fprintf(stderr, MTxt[7],char(Variant), Version);
        fprintf(stderr, "\n");
        fprintf(stderr, MTxt[6],SASize,MAX_O,MaxOrder);
        exit(-1);
    }

    int opt;
    while ((opt=getopt(argc, argv, "cdfhkqvzLV123456789m:o:r:f:")) != -1) {
        switch(opt) {
            case 'c': //stdout
                WriteStdout=PPMD_TRUE;
                break;
            case 'd': //Decode
                EncodeFlag = PPMD_FALSE;
                break;
            case 'f': //Force
                Overwrite = PPMD_TRUE;
                break;
            case 'h': //Help test
                fprintf(stderr, MTxt[7],char(Variant), Version);
                fprintf(stderr, "\n");
                fprintf(stderr, MTxt[6],SASize,MAX_O,MaxOrder);
                return 0;
                break;
            case 'k': //Keep input
                DeleteFile=PPMD_FALSE;
                break;
            case 'q': //quiet
                
                break;
            case 'v': //verbose
                Verbose = PPMD_TRUE;
                break;
            case 'z': //Encode
                EncodeFlag = PPMD_TRUE;
                break;
            case 'L': //Licence

                break;
            case 'V': //Version
                fprintf(stderr, MTxt[7],char(Variant), Version);
                return 0;
                break;
            case '1': //Fast
            case '2': //Fast
            case '3': //Fast
            case '4': //Fast
                if (MaxOrder==-1) MaxOrder=4;
                if (MRMethod==-1) MRMethod=MRM_RESTART;
                break;
            case '5': //Normal
                break;
            case '6': //MAXXX
            case '7': //MAXXX
            case '8': //MAXXX
            case '9': //MAXXX
                if (MaxOrder==-1) MaxOrder=16;
                if (MRMethod==-1) MRMethod=MRM_CUT_OFF;
                if (SASize==-1) SASize=20;
                break;
            case 'm': //Memory
                SASize = atoi(optarg);
                break;
            case 'o': //Order
                if (atoi(optarg)<2 || atoi(optarg)>MAX_O) {
                    fprintf(stderr, MTxt[5], "-o", atoi(optarg));
                    exit (-1);
                } else
                    MaxOrder = atoi(optarg);
                break;
            case 'r': //Restart
                if (atoi(optarg)<0 || atoi(optarg)>2) {
                    fprintf(stderr, MTxt[5], "-r", atoi(optarg));
                    exit (-1);
                } else
                    MRMethod = atoi(optarg);
                break;
        }
    }

    //Now setup defaults
    if (MaxOrder==-1) MaxOrder=8;
    if (MRMethod==-1) MRMethod=MRM_RESTART;
    if (SASize==-1) SASize=10;
    
    // If no files given then encoding from stdin to stdout
    if ( (optind == argc) && (argc > 1)) {
        BOOL error=0;
        if ( EncodeFlag )                   
	        error = EncodeFile("-","-", MaxOrder,SASize,(MR_METHOD)MRMethod,Verbose);
        else                                
	        error = DecodeFile("-","-", Verbose);
    }

    char *ArcFile;
    for (i=optind; i<argc; i++) {
        if (!WriteStdout) {
            ArcFile = new char[strlen(argv[i])+5];
            if (ChangeExt(argv[i], ArcFile, EncodeFlag) == PPMD_FALSE)
                fprintf(stderr, MTxt[8], argv[i], ArcFile); //Warn that we need to guess a filename
        } else {
            ArcFile = new char[2];
            strcpy(ArcFile, "-");
        }

        BOOL success=0;
        if ( EncodeFlag )                   
	        success = EncodeFile(argv[i], ArcFile, MaxOrder,SASize,(MR_METHOD)MRMethod,Overwrite,Verbose);
        else                                
	        success = DecodeFile(argv[i], ArcFile, Overwrite, Verbose);
        if ( DeleteFile && success)                   
	        RemoveFile(argv[i]);

        delete ArcFile;
    }

    
    return 0;
}


