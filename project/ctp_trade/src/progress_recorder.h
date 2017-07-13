/*****************************************************************************
 Nautilus Module ctp_trade Copyright (c) 2016. All Rights Reserved.

 FileName: progress_recorder.h
 Version: 1.0
 Date: 2016.03.21

 History:
 david wang     2016.03.21   1.0     Create
 ******************************************************************************/

#ifndef __NAUT_CPTTRADE_PROGRESS_RECORDER_H__
#define __NAUT_CTPTRADE_PROGRESS_RECORDER_H__

#include "base/thread.h"
#include "base/file.h"
#include "base/event.h"
#include "base/mqueue.h"

namespace ctp
{

enum ORDER_PROGRESS
{
	ORDER_RECV = 1,
	ORDER_PRE_PROCESSING,
	ORDER_PRE_PREOCESSED,
	ORDER_SENDDING_TO_SERVER,
	ORDER_FINISH,
};

#pragma pack(push, 1)

struct progress_message
{
	void* data;
	int size;
};

#pragma pack(pop)

struct progress_info
{
	std::string broker;
	std::string account;
	long recv_index;
	long processed_index;

	progress_info()
		: recv_index(-1)
		, processed_index(-1)
	{}
};

class progress_recorder
	: public base::process_thread
{
protected:
	progress_recorder();
	virtual ~progress_recorder();

public:
	static progress_recorder& shared_instance();
	static void release_shared_instance();

	void record_progress(const char* broker, const char* account,
			const char* orderid, long msg_index, int progress);
	progress_info* get_progress_info(const char* key);

public:
	int init(const char* log_file);
	int release();

protected:
	int init_progress_info();

protected:
	virtual void run();

	void post(progress_message& msg);
	int get(progress_message& msg);
	void release_messages();

private:
	static progress_recorder* shared_instance_;

	bool initialized_;
	base::srt_queue<progress_message>* msg_queue_;
	base::event* msg_event_;

	VBASE_HASH_MAP<std::string, progress_info*> map_progress_info_;

	std::string log_file_;
	base::file f_;
};

}

#endif  //__NAUT_CPTTRADE_PROGRESS_RECORDER_H__