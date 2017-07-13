/*****************************************************************************
 Nautilus Module ctp_trade Copyright (c) 2016. All Rights Reserved.

 FileName: progress_recorder.cpp
 Version: 1.0
 Date: 2016.03.21

 History:
 david wang     2016.03.21   1.0     Create
 ******************************************************************************/

#include "progress_recorder.h"
#include <string>
#include "base/base.h"
#include "base/trace.h"
#include "base/util.h"
#include "common.h"
#include <string.h>
#include <stdio.h>

namespace ctp
{

progress_recorder* progress_recorder::shared_instance_ = NULL;
progress_recorder::progress_recorder()
	: initialized_(false)
	, msg_queue_(NULL)
	, msg_event_(NULL)
{

}

progress_recorder::~progress_recorder()
{
	release();
}

progress_recorder& progress_recorder::shared_instance()
{
	if (shared_instance_ == NULL) {
		shared_instance_ = new progress_recorder();
	}
	return *shared_instance_;
}

void progress_recorder::release_shared_instance()
{
	if (shared_instance_ != NULL) {
		shared_instance_->release();
		delete shared_instance_;
		shared_instance_ = NULL;
	}
}

void progress_recorder::record_progress(const char* broker,
		const char* account, const char* orderid, long msg_index, int progress)
{
	if (!initialized_) {
		return;
	}

	int broker_size = strlen(broker) + 1;
	int account_size = strlen(account) + 1;
	int orderid_size = strlen(orderid) + 1;

	int size = broker_size + account_size + orderid_size + sizeof(long) + sizeof(int);
	char* data = new char[size + 1];

	int offset = 0;
	memcpy(data + offset, broker, broker_size);
	offset += broker_size;
	memcpy(data + offset, account, account_size);
	offset += account_size;
	memcpy(data + offset, orderid, orderid_size);
	offset += orderid_size;
	memcpy(data + offset, &msg_index, sizeof(msg_index));
	offset += sizeof(msg_index);
	memcpy(data + offset, &progress, sizeof(progress));

	progress_message msg;
	msg.data = data;
	msg.size = size;
	post(msg);
}

progress_info* progress_recorder::get_progress_info(const char* key)
{
	if (!initialized_) {
		return NULL;
	}
	if (map_progress_info_.find(key) == map_progress_info_.end()) {
		return NULL;
	}
	else {
		return map_progress_info_[key];
	}
}

void progress_recorder::run()
{
	while (is_running_)
	{
		progress_message msg;
		if (get(msg) != 0) {
			msg_event_->reset();
			msg_event_->wait(20);
			continue;
		}

		int crc = base::util::calculate_crc(&msg.size, sizeof(msg.size), 0);
		crc = base::util::calculate_crc(msg.data, msg.size, crc);

		f_.write((const unsigned char*)&crc, sizeof(int));
		f_.write((const unsigned char*)&msg.size, sizeof(msg.size));
		f_.write((const unsigned char*)msg.data, msg.size);
	}
}

int progress_recorder::init(const char* log_file)
{
	if (initialized_) {
		return NAUT_AT_S_OK;
	}

	int ret = NAUT_AT_S_OK;

	if (f_.open(log_file) < 0) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_OPEN_PROGRESS_LOG_FILE_FAILED,
				"open progress log file failed, file path: '%s'", log_file);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_OPEN_PROGRESS_LOG_FILE_FAILED, end);
	}

	ret = init_progress_info();
	CHECK_LABEL(ret, end);

	log_file_ = log_file;
	msg_queue_ = new base::srt_queue<progress_message>(10);
	msg_queue_->init();
	msg_event_ = new base::event();

	base::process_thread::start();

end:
	if (BFAILED(ret)) {
		f_.close();
		release();
	}
	else {
		initialized_ = true;
	}
	return ret;
}

int progress_recorder::release()
{
	initialized_ = false;

	base::process_thread::stop();
	f_.close();

	release_messages();

	if (msg_queue_ != NULL) {
		delete msg_queue_;
		msg_queue_ = NULL;
	}

	if (msg_event_ != NULL) {
		delete msg_event_;
		msg_event_ = NULL;
	}

	VBASE_HASH_MAP<std::string, progress_info*>::iterator mit = map_progress_info_.begin();
	for ( ; mit != map_progress_info_.end(); mit++) {
		delete mit->second;
	}
	map_progress_info_.clear();

	return NAUT_AT_S_OK;
}

int progress_recorder::init_progress_info()
{
	int ret = NAUT_AT_S_OK;

	LABEL_SCOPE_START;

	f_.seek(0, base::FILE_SEEK_SET);

	const int buffer_size = 8192;
	char buffer[buffer_size];
	char key[128];

	int offset_r = 0;
	int offset_w = 0;
	int int_size = sizeof(int);

	do {
		int read_size = f_.read((base::byte*)buffer + offset_w, buffer_size - offset_w);
		if (read_size <= 0) {
			break;
		}
		offset_w += read_size;

		while (true) {
			if (offset_w - offset_r < 2 * int_size) {
				break;
			}

			int crc = *((int*)(buffer + offset_r));
			int size = *((int*)(buffer + offset_r + int_size));
			if (offset_w - offset_r < 2 * int_size + size) {
				break;
			}

			if (base::util::calculate_crc(buffer + offset_r + int_size, size + int_size, 0) != crc) {
				TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_PROGRESS_INFO_CRC_MISMATCHED,
						"crc of the progress info mismatched");
				ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_PROGRESS_INFO_CRC_MISMATCHED, end);
			}

			int offset = offset_r + 2 * int_size;
			char* broker = (char*)buffer + offset;
			offset += strlen(broker) + 1;
			char* account = (char*)buffer + offset;
			offset += strlen(account) + 1;
			char* orderid = (char*)buffer + offset;
			offset += strlen(orderid) + 1;
			long msg_index = *(long*)(buffer + offset);
			offset += sizeof(long);
			int progress = *(int*)(buffer + offset);

			sprintf(key, "%s_%s", broker, account);
			if (map_progress_info_.find(key) == map_progress_info_.end()) {
				progress_info* pinfo = new progress_info();
				pinfo->broker = broker;
				pinfo->account = account;
				pinfo->recv_index = 0;
				pinfo->processed_index = -1;
				map_progress_info_[key] = pinfo;
			}
			progress_info* pinfo = map_progress_info_[key];
			if (pinfo->recv_index < msg_index) {
				pinfo->recv_index = msg_index;
			}
			if (progress > ORDER_RECV) {
				if (pinfo->processed_index < msg_index) {
					pinfo->processed_index = msg_index;
				}
			}

			offset_r += (size + 2 * int_size);
		}

		/* move to unhandled data */
		memmove((base::byte*)buffer, (base::byte*)buffer + offset_r, offset_w - offset_r);
		offset_w -= offset_r;
		offset_r = 0;
	} while(true);

	/* indicate that there exists incomplete record */
	if (offset_w != 0) {
		f_.seek(f_.size() - offset_w, base::FILE_SEEK_SET);
	}
	else {
		f_.seek(f_.size(), base::FILE_SEEK_SET);
	}

	LABEL_SCOPE_END;

end:
	return ret;
}

void progress_recorder::post(progress_message& msg)
{
	msg_queue_->push(msg);
	msg_event_->set();
}

int progress_recorder::get(progress_message& msg)
{
	if (msg_queue_->pop(msg)) {
		return 0;
	}
	else {
		return -1;
	}
}

void progress_recorder::release_messages()
{
	if (msg_queue_ != NULL) {
		progress_message msg;
		while (msg_queue_->pop(msg)) {
			delete[](char*)msg.data;
		}
	}
}

}
