/***************************************************************************** 
a50-zd Module Copyright (c) 2015. All Rights Reserved. 
FileName: product_business_deal.cpp
Version: 1.0 
Date: 2015.10.26 
History: cwm     2015.10.26   1.0     Create 
******************************************************************************/

#include "product_business_deal.h"
#include "process.h"
#include "base/trace.h"
#include <math.h>

namespace serverframe
{

int product_business_deal::check_deal_is_exist(base::dictionary &dict)
{
    int ret = NAUT_S_OK;

    LABEL_SCOPE_START;


    LABEL_SCOPE_END;
end:
    return ret;
}

int product_business_deal::get_stocker_attr(base::dictionary &d,const std::string &user_name)
{
    int ret = NAUT_S_OK;
    LABEL_SCOPE_START;

    LABEL_SCOPE_END;

    return ret;
}

int product_business_deal::add_deal(base::dictionary &dict, database::db_instance* pconn)
{
    int ret = NAUT_S_OK;
    LABEL_SCOPE_START;

    LABEL_SCOPE_END;

end:
    return ret;
}

}
