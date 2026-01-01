#pragma once

#define DELETE_COPY(class_name)\
class_name(const class_name &_Copy) = delete;\
class_name &operator=(const class_name &_Copy) = delete;\

#define DELETE_MOVE(class_name)\
class_name(class_name &&_Move) = delete;\
class_name &operator=(class_name &&_Move) = delete;

#define DELETE_CPMV(class_name)\
DELETE_COPY(class_name)\
DELETE_MOVE(class_name)




