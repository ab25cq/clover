; ModuleID = 'llclover'

declare i32 @llget_int_value_from_stack(i32, void*)

declare i8 @llget_byte_value_from_stack(i32, void*)

declare i32 @llget_type_object_from_stack(i32, void*)

declare void @llinc_stack_pointer(i32, void*)

declare void @llcreate_int_object(i32, i32, void*)

declare void @vm_mutex_lock()

declare void @vm_mutex_unlock()

define i32 @"int._constructor(int)"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
}

define i32 @"int.+(int)"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
  %calltmp = call void @vm_mutex_lock()
  %calltmp1 = call i32 @llget_int_value_from_stack(i32 -1, void* %info)
  %calltmp2 = call i32 @llget_int_value_from_stack(i32 -2, void* %info)
  %addtmp = add i32 %calltmp1, %calltmp2
  %calltmp3 = call i32 @llget_type_object_from_stack(i32 -2, void* %info)
  %calltmp4 = call void @llinc_stack_pointer(i32 -2, void* %info)
  %calltmp5 = call void @llcreate_int_object(i32 %addtmp, i32 %calltmp3, void* %info)
  %calltmp6 = call void @llinc_stack_pointer(i32 1, void* %info)
  %calltmp7 = call void @vm_mutex_unlock()
  ret void %calltmp5
}

define i32 @"int.-(int)"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
  %calltmp = call void @vm_mutex_lock()
  %calltmp1 = call i32 @llget_int_value_from_stack(i32 -1, void* %info)
  %calltmp2 = call i32 @llget_int_value_from_stack(i32 -2, void* %info)
  %addtmp = sub i32 %calltmp1, %calltmp2
  %calltmp3 = call i32 @llget_type_object_from_stack(i32 -2, void* %info)
  %calltmp4 = call void @llinc_stack_pointer(i32 -2, void* %info)
  %calltmp5 = call void @llcreate_int_object(i32 %addtmp, i32 %calltmp3, void* %info)
  %calltmp6 = call void @llinc_stack_pointer(i32 1, void* %info)
  %calltmp7 = call void @vm_mutex_unlock()
  ret void %calltmp5
}

define i32 @"int.*(int)"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
  %calltmp = call void @vm_mutex_lock()
  %calltmp1 = call i32 @llget_int_value_from_stack(i32 -1, void* %info)
  %calltmp2 = call i32 @llget_int_value_from_stack(i32 -2, void* %info)
  %addtmp = mul i32 %calltmp1, %calltmp2
  %calltmp3 = call i32 @llget_type_object_from_stack(i32 -2, void* %info)
  %calltmp4 = call void @llinc_stack_pointer(i32 -2, void* %info)
  %calltmp5 = call void @llcreate_int_object(i32 %addtmp, i32 %calltmp3, void* %info)
  %calltmp6 = call void @llinc_stack_pointer(i32 1, void* %info)
  %calltmp7 = call void @vm_mutex_unlock()
  ret void %calltmp5
}

define i32 @"int.%(int)"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
  %calltmp = call void @vm_mutex_lock()
  %calltmp1 = call i32 @llget_int_value_from_stack(i32 -1, void* %info)
  %calltmp2 = call i32 @llget_int_value_from_stack(i32 -2, void* %info)
  %addtmp = srem i32 %calltmp1, %calltmp2
  %calltmp3 = call i32 @llget_type_object_from_stack(i32 -2, void* %info)
  %calltmp4 = call void @llinc_stack_pointer(i32 -2, void* %info)
  %calltmp5 = call void @llcreate_int_object(i32 %addtmp, i32 %calltmp3, void* %info)
  %calltmp6 = call void @llinc_stack_pointer(i32 1, void* %info)
  %calltmp7 = call void @vm_mutex_unlock()
  ret void %calltmp5
}

define i32 @"int./(int)"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
  ret i32 0
}

define i32 @"int.<<(int)"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
  ret i32 0
}

define i32 @"int.>>(int)"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
  ret i32 0
}

define i32 @"int.==(int)"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
  ret i32 0
}

define i32 @"int.!=(int)"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
  ret i32 0
}

define i32 @"int.<(int)"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
  ret i32 0
}

define i32 @"int.<=(int)"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
  ret i32 0
}

define i32 @"int.>(int)"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
  ret i32 0
}

define i32 @"int.>=(int)"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
  ret i32 0
}

define i32 @"int.&(int)"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
  ret i32 0
}

define i32 @"int.^(int)"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
  ret i32 0
}

define i32 @"int.|(int)"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
  ret i32 0
}

define i32 @"int.~()"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
  ret i32 0
}

define i32 @"int.++()"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
  ret i32 0
}

define i32 @"int.++2()"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
  ret i32 0
}

define i32 @"int.--()"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
  ret i32 0
}

define i32 @"int.--2()"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
  ret i32 0
}

define i32 @"int.+=(int)"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
  %calltmp = call void @vm_mutex_lock()
  %calltmp1 = call i32 @llget_int_value_from_stack(i32 -1, void* %info)
  %calltmp2 = call i32 @llget_int_value_from_stack(i32 -2, void* %info)
  %addtmp = add i32 %calltmp1, %calltmp2
  %calltmp3 = call i32 @llget_type_object_from_stack(i32 -2, void* %info)
  %calltmp4 = call void @llinc_stack_pointer(i32 -2, void* %info)
  %calltmp5 = call void @llcreate_int_object(i32 %addtmp, i32 %calltmp3, void* %info)
  %calltmp6 = call void @llinc_stack_pointer(i32 1, void* %info)
  %calltmp7 = call void @vm_mutex_unlock()
  ret void %calltmp5
}

define i32 @"int.-=(int)"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
  %calltmp = call void @vm_mutex_lock()
  %calltmp1 = call i32 @llget_int_value_from_stack(i32 -1, void* %info)
  %calltmp2 = call i32 @llget_int_value_from_stack(i32 -2, void* %info)
  %addtmp = sub i32 %calltmp1, %calltmp2
  %calltmp3 = call i32 @llget_type_object_from_stack(i32 -2, void* %info)
  %calltmp4 = call void @llinc_stack_pointer(i32 -2, void* %info)
  %calltmp5 = call void @llcreate_int_object(i32 %addtmp, i32 %calltmp3, void* %info)
  %calltmp6 = call void @llinc_stack_pointer(i32 1, void* %info)
  %calltmp7 = call void @vm_mutex_unlock()
  ret void %calltmp5
}

define i32 @"int.*=(int)"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
  %calltmp = call void @vm_mutex_lock()
  %calltmp1 = call i32 @llget_int_value_from_stack(i32 -1, void* %info)
  %calltmp2 = call i32 @llget_int_value_from_stack(i32 -2, void* %info)
  %addtmp = mul i32 %calltmp1, %calltmp2
  %calltmp3 = call i32 @llget_type_object_from_stack(i32 -2, void* %info)
  %calltmp4 = call void @llinc_stack_pointer(i32 -2, void* %info)
  %calltmp5 = call void @llcreate_int_object(i32 %addtmp, i32 %calltmp3, void* %info)
  %calltmp6 = call void @llinc_stack_pointer(i32 1, void* %info)
  %calltmp7 = call void @vm_mutex_unlock()
  ret void %calltmp5
}

define i32 @"int./=(int)"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
  ret i32 0
}

define i32 @"int.%=(int)"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
  %calltmp = call void @vm_mutex_lock()
  %calltmp1 = call i32 @llget_int_value_from_stack(i32 -1, void* %info)
  %calltmp2 = call i32 @llget_int_value_from_stack(i32 -2, void* %info)
  %addtmp = srem i32 %calltmp1, %calltmp2
  %calltmp3 = call i32 @llget_type_object_from_stack(i32 -2, void* %info)
  %calltmp4 = call void @llinc_stack_pointer(i32 -2, void* %info)
  %calltmp5 = call void @llcreate_int_object(i32 %addtmp, i32 %calltmp3, void* %info)
  %calltmp6 = call void @llinc_stack_pointer(i32 1, void* %info)
  %calltmp7 = call void @vm_mutex_unlock()
  ret void %calltmp5
}

define i32 @"int.<<=(int)"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
  ret i32 0
}

define i32 @"int.>>=(int)"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
  ret i32 0
}

define i32 @"int.&=(int)"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
  ret i32 0
}

define i32 @"int.^=(int)"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
  ret i32 0
}

define i32 @"int.|=(int)"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
  ret i32 0
}

define i32 @"int.toLong()"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
}

define i32 @"int.toShort()"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
}

define i32 @"int.toByte()"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
}

define i32 @"int.toUInt()"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
}

define i32 @"int.toFloat()"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
}

define i32 @"int.toDouble()"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
}

define i32 @"int.toChar()"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
}

define i32 @"int.toString()"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
}

define i32 @"int.times()bool{int}"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
  ret i32 0
}

define i32 @"int.times()bool{}"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
  ret i32 0
}

define i32 @"int.toInt()"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
  ret i32 0
}

define i32 @"int.toBool()"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
  ret i32 0
}

define i32 @"int.setValue(int)"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
}

define i32 @"int.hashValue()"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
  ret i32 0
}

define i32 @"int.clone()"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
  ret i32 0
}

define i32 @"int.dup()"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
  ret i32 0
}

define i32 @"int.toSignal()"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
  ret i32 0
}

define i32 @"int.toWaitOption()"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
  ret i32 0
}

define i32 @"int.toAccessMode()"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
  ret i32 0
}

define i32 @"int.toFnmatchFlags()"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
  ret i32 0
}

define i32 @"int.toFileLockOperation()"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
  ret i32 0
}

define i32 @"int.toFileMode()"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
  ret i32 0
}

define i32 @"int.to_pid_t()"(void** %stack_ptr, void* %lvar, void* %info, i32 %vm_type, void* %klass) {
entry:
  ret i32 0
}
