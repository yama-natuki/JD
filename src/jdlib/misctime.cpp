// ライセンス: GPL2

//#define _DEBUG
#include "jddebug.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "misctime.h"

#include <sstream>
#include <cstdio>
#include <cstring>
#include <time.h>
#include <sys/time.h>

#ifdef _WIN32
// not exist _r suffix functions in mingw time.h
#define localtime_r( _clock, _result ) \
        ( *(_result) = *localtime( (_clock) ), \
          (_result) )
#endif

//
// gettimeofday()の秒を文字列で取得
//
std::string MISC::get_sec_str()
{
	std::ostringstream sec_str;

	struct timeval tv;
    struct timezone tz;
    gettimeofday( &tv, &tz );

    sec_str << tv.tv_sec;

    return sec_str.str();
}


//
// timeval を str に変換
//
std::string MISC::timevaltostr( struct timeval& tv )
{
    std::ostringstream sstr;
    sstr << ( tv.tv_sec >> 16 ) << " " << ( tv.tv_sec & 0xffff ) << " " << tv.tv_usec;
    return sstr.str();
}


//
// 時刻を紀元からの経過秒に直す
//
time_t MISC::datetotime( const std::string& date )
{
    if( date.empty() ) return 0;

    struct tm tm_out;
    memset( &tm_out, 0, sizeof( struct tm ) );

#ifdef _WIN32
    {
        char month[4];
        char monthes[][4] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                              "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"  };
        if (sscanf(date.c_str(), "%*3s, %2d %3s %4d %2d:%2d:%2d",
            &tm_out.tm_mday, month, &tm_out.tm_year,
            &tm_out.tm_hour, &tm_out.tm_min, &tm_out.tm_sec) == 6 ) return 0;
        for (int i=0; i<12; i++)
        {
            if (strcasecmp(monthes[i], month) == 0)
            {
                tm_out.tm_mon = i;
                break;
            }
        }
    }
#else
    if( strptime( date.c_str(), "%a, %d %b %Y %T %Z", &tm_out ) == NULL ) return 0;
#endif

#ifdef USE_MKTIME
    time_t t_ret = mktime( &tm_out );
#else
    time_t t_ret = timegm( &tm_out );
#endif

#ifdef _DEBUG
    std::cout << "MISC::datetotime " << date << " -> " << t_ret << std::endl;
#endif

    return t_ret;
}


//
// time_t を月日の文字列に変換
//
std::string MISC::timettostr( time_t time_from )
{
    char str_ret[ 64 ];
    str_ret[ 0 ] = '\0';

    struct tm tm_tmp;
    if( localtime_r( &time_from, &tm_tmp ) ){

        snprintf( str_ret, 64, "%d/%02d/%02d %02d:%02d",
                  ( 1900 + tm_tmp.tm_year ), ( 1 + tm_tmp.tm_mon ), tm_tmp.tm_mday, tm_tmp.tm_hour, tm_tmp.tm_min );
    }

#ifdef _DEBUG
    std::cout << "MISC::timettostr " << time_from << " -> " << str_ret << std::endl;
#endif

    return str_ret;
}


#ifdef NO_TIMEGM
//
// timegm
//
// Solarisの場合はtimegmが存在しないため、代替コードを宣言する(by kohju)
// 原典：linux の man timegm
//
time_t timegm (struct tm *tm)
{
    time_t ret;
    char *tz;

    tz = getenv("TZ");
    setenv("TZ", "", 1);
    tzset();
    ret = mktime(tm);
    if (tz)
 	setenv("TZ", tz, 1);
    else
 	unsetenv("TZ");
    tzset();
    return ret;
}
#endif
