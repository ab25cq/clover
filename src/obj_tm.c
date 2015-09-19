#include "clover.h"
#include "common.h"
#include <time.h>

BOOL tm_tm(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject time;
    CLObject new_obj;
    struct tm* time_struct;
    time_t time_value;
    CLObject ovalue;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "tm", info)) {
        return FALSE;
    }

    time = (lvar+1)->mObjectValue.mValue;
    if(!check_type_with_class_name(time, "time_t", info)) {
        return FALSE;
    }

    time_value = get_value_with_size(sizeof(time_t), time);

    time_struct = localtime(&time_value);

    ovalue = create_int_object(time_struct->tm_sec);

    CLUSEROBJECT(self)->mFields[0].mObjectValue.mValue = ovalue;

    ovalue = create_int_object(time_struct->tm_min);
    CLUSEROBJECT(self)->mFields[1].mObjectValue.mValue = ovalue;

    ovalue = create_int_object(time_struct->tm_hour);
    CLUSEROBJECT(self)->mFields[2].mObjectValue.mValue = ovalue;
    
    ovalue = create_int_object(time_struct->tm_mday);
    CLUSEROBJECT(self)->mFields[3].mObjectValue.mValue = ovalue;

    ovalue = create_int_object(time_struct->tm_mon);
    CLUSEROBJECT(self)->mFields[4].mObjectValue.mValue = ovalue;

    ovalue = create_int_object(time_struct->tm_year);
    CLUSEROBJECT(self)->mFields[5].mObjectValue.mValue = ovalue;

    ovalue = create_int_object(time_struct->tm_wday);
    CLUSEROBJECT(self)->mFields[6].mObjectValue.mValue = ovalue;

    ovalue = create_int_object(time_struct->tm_yday);
    CLUSEROBJECT(self)->mFields[7].mObjectValue.mValue = ovalue;

    ovalue = create_bool_object(time_struct->tm_isdst > 0);
    CLUSEROBJECT(self)->mFields[8].mObjectValue.mValue = ovalue;

    (*stack_ptr)->mObjectValue.mValue = self;
    (*stack_ptr)++;

    return TRUE;
}
