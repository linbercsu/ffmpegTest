/*
 * rc_util_compute_time.h
 *
 *  Created on: 2016-8-24
 *      Author:
 */

#ifndef RC_UTIL_COMPUTE_TIME_H_
#define RC_UTIL_COMPUTE_TIME_H_
#ifdef __cplusplus
extern "C"
{
#endif
// 计时对象声明;
struct rc_time_context;
typedef struct rc_time_context rc_time_context;

// 生成计时对象；
rc_time_context *rc_time_create_context();

// 销毁计时对象；
void rc_time_destory_context(rc_time_context *_pContext);

// 记下开始时间;
void rc_time_start(rc_time_context *_pContext);

// 记下结束时间;
void rc_time_stop(rc_time_context *_pContext);

// 获取开始到结束的耗时（单位：微秒）
long long int rc_time_get_cost(rc_time_context *_pContext);


////////////////////////////////////////////////////////////
// 统计对象声明;
struct rc_statistic_context
{
    long long int __i64TotalCost;
    long long int __i64LastCost;
    int _iCount;
};

typedef struct rc_statistic_context rc_statistic_context;


// 重置统计数据;
void rc_time_statistic_reset(rc_statistic_context *_pStatistic);

// 加入耗时数据；
void rc_time_statistic_add_cost(rc_statistic_context *_pStatistic, long long int _i64Cost);

// 通过计时对象加入耗时数据;
void rc_time_statistic_add_context(rc_statistic_context *_pStatistic, rc_time_context *_pContext);

// 获取统计次数;
int rc_time_statistic_get_count(rc_statistic_context *_pStatistic);

// 获取总耗时;
long long int rc_time_statistic_get_total_cost(rc_statistic_context *_pStatistic);

// 获取平均耗时;
long long int rc_time_statistic_get_avg_cost(rc_statistic_context *_pStatistic);

// 获取输出信息字符串;
const char * rc_time_statisttic_get_log_str(  rc_statistic_context * _pStatistic, char * buf);
//////////////////////////////////////////////////////////////
// 全局计时开始;
void rc_gloable_time_start();

// 全局计时结束;
void rc_gloable_time_stop();

// 全局计时重置;
void rc_gloable_statistic_reset();

// 加入统计;
void rc_gloable_statistic_add();

// 获取测试输出日志;
const char *rc_gloable_time_log_str();

#define RC_LOG_MACRO_ENABLE 1
#if RC_LOG_MACRO_ENABLE
#define RC_STATISTIC_LOG_BEGIN {\
                            static rc_time_context * slv_timeContex = rc_time_create_context();\
                            static rc_statistic_context slv_statisticContex;\
                            char RC_LOG_STR[1024] = {0};\
                            rc_time_start( slv_timeContex);

#define RC_STATISTIC_LOG_ADD    rc_time_stop( slv_timeContex);\
                                rc_time_statistic_add_context( &slv_statisticContex, slv_timeContex);\
                                rc_time_statisttic_get_log_str( &slv_statisticContex, RC_LOG_STR);

#define RC_STATISTIC_LOG_END    };

#else

#define RC_STATISTIC_LOG_BEGIN

#define RC_STATISTIC_LOG_ADD

#define RC_STATISTIC_LOG_END

#endif

#ifdef __cplusplus
};
#endif

#endif /* RC_UTIL_COMPUTE_TIME_H_ */
