#ifndef __PARSE_H__
#define __PARSE_H__

#define IN
#define OUT

#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "protocal.h"

namespace serverframe
{

inline int as_int(const char* buf)
{
    int i= 0;
    memcpy(&i, buf, sizeof(int));
    return i;
}

//encode
inline void encode(IN unsigned int clientid, IN const char* buf, IN int len,
        OUT std::string& result)
{
    result.append(buf, len);
    result.append(reinterpret_cast<char*>(&clientid), sizeof(unsigned int));
}

//decode
inline void decode(IN const std::string& buf, /*common::MSG_HEADER &msg_head, */
        OUT std::string &message, OUT unsigned int &clientid)
{
    //memcpy(&msg_head, result.c_str(), sizeof(common::MSG_HEADER));
    message.append(buf, sizeof(MSG_HEADER),
            buf.size() - sizeof(MSG_HEADER) - sizeof(unsigned int));
    clientid = as_int(buf.c_str() + buf.size() - sizeof(unsigned int));
}

inline int get_message_type( const std::string& buf )
{
    MSG_HEADER msg_head;
    if (!buf.empty()) {
        memcpy(&msg_head, buf.c_str(), sizeof(MSG_HEADER));
    }
    return msg_head.type;
}
}
#endif
