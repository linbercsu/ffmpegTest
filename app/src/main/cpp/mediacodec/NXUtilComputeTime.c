/*
 * rc_util_compute_time.c
 *
 *  Created on: 2016-8-24
 *      Author:
 */

#include "NXUtilComputeTime.h"
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

struct rc_time_context
{
	struct timeval 	__start;
	struct timeval 	__end;
	int 			__frameCount;
	long long int 	__i64TotalCost;
};


static long long int compute_cost( struct timeval * _start, struct timeval * _end)
{
	long sec 	= _end->tv_sec - _start->tv_sec;
	long usec 	= _end->tv_usec - _start->tv_usec;
	long long int cost = sec*1000000 + usec;
	return cost;
}


rc_time_context * rc_time_create_context()
{
	rc_time_context * pCtx = (rc_time_context *)malloc(sizeof(rc_time_context));
	return pCtx;
}

void rc_time_destory_context( rc_time_context * _pContext)
{
	if( NULL != _pContext)
	{
		free( _pContext);
		_pContext = NULL;
	}
}

void rc_time_start( rc_time_context * _pContext)
{
	if( NULL == _pContext) return;

	gettimeofday(&_pContext->__start, NULL);
}

void rc_time_stop( rc_time_context * _pContext)
{
	if( NULL == _pContext) return;

	gettimeofday(&_pContext->__end, NULL);

}

long long int rc_time_get_cost( rc_time_context * _pContext)
{
	if( NULL == _pContext) return -1;

	return compute_cost( &_pContext->__start, &_pContext->__end);
}


// 重置统计数据;
void rc_time_statistic_reset( rc_statistic_context * _pStatistic)
{
	if( NULL != _pStatistic)
	{
		memset( _pStatistic, 0, sizeof(rc_statistic_context));
	}
}

// 加入耗时数据；
void rc_time_statistic_add_cost( rc_statistic_context * _pStatistic, long long int _i64Cost)
{
	if( NULL == _pStatistic) return;
	_pStatistic->__i64LastCost = _i64Cost;
	_pStatistic->_iCount += 1;
	_pStatistic->__i64TotalCost += _i64Cost;
}

// 通过计时对象加入耗时数据;
void rc_time_statistic_add_context( rc_statistic_context * _pStatistic, rc_time_context * _pContext)
{
	rc_time_statistic_add_cost( _pStatistic, rc_time_get_cost(_pContext));
}

// 获取统计次数;
int	 rc_time_statistic_get_count( rc_statistic_context * _pStatistic)
{
	if( NULL == _pStatistic) return 0;
	return _pStatistic->_iCount;
}

// 获取总耗时;
long long int rc_time_statistic_get_total_cost( rc_statistic_context * _pStatistic)
{
	if( NULL == _pStatistic) return 0;
	return _pStatistic->__i64TotalCost;

}

// 获取平均耗时;
long long int rc_time_statistic_get_avg_cost( rc_statistic_context * _pStatistic)
{
	if( NULL == _pStatistic) return 0;
	if( 0 == _pStatistic->_iCount) return 0;
	return(_pStatistic->__i64TotalCost/_pStatistic->_iCount);
}

const char * rc_time_statisttic_get_log_str(  rc_statistic_context * _pStatistic, char * buf)
{
	if( NULL == _pStatistic) return 0;
	if( 0 == _pStatistic->_iCount) return 0;
	int count = rc_time_statistic_get_count( _pStatistic);
	long long int total = rc_time_statistic_get_total_cost( _pStatistic);
	long long int avg	= rc_time_statistic_get_avg_cost( _pStatistic);
	long long int last	= _pStatistic->__i64LastCost;
	sprintf( buf, "index:[%6d] last:[%7lld]us avg:[%7lld]us total:[%10lld]", count, last, avg, total);
	return buf;
}

static rc_time_context		sv_timeCtx = {0};
static rc_statistic_context sv_statisticCtx = {0};
static char sv_arrayLogStr[256] = {0};

// 全局计时开始;
void rc_gloable_time_start()
{
	rc_time_start( &sv_timeCtx);
}

// 全局计时结束;
void rc_gloable_time_stop()
{
	rc_time_stop( &sv_timeCtx);
}

// 全局计时重置;
void rc_gloable_statistic_reset()
{
	rc_time_statistic_reset( &sv_statisticCtx);
}

void rc_gloable_statistic_add()
{
	rc_time_statistic_add_context( &sv_statisticCtx, &sv_timeCtx);
}

// 获取测试输出日志;
const char* rc_gloable_time_log_str()
{
	int count = rc_time_statistic_get_count( &sv_statisticCtx);
	long long int total = rc_time_statistic_get_total_cost( &sv_statisticCtx);
	long long int avg	= rc_time_statistic_get_avg_cost( &sv_statisticCtx);
	long long int last	= sv_statisticCtx.__i64LastCost;
	sprintf( sv_arrayLogStr, "index:[%6d] last:[%7lld]us avg:[%7lld]us total:[%10lld]", count, last, avg, total);
	return sv_arrayLogStr;
}
