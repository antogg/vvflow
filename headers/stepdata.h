#ifndef STEPDATA_H_
#define STEPDATA_H_

#include "core.h"
#include <iostream>

class Stepdata
{
	public:
		Stepdata(Space* s_);
		void create(const char *format);
		void write();
		void close();

	private:
		void attribute_write(const char *name, const char *str);
		int create_dataset(int loc_id, const char *name, int cols);
		void append(int dataspace_hid, const void* buf);
		void append(int dataspace_hid, double value);
		void append(int dataspace_hid, TVec3D value);
		Space *S;
		int file_hid;
		int string_hid;
		int DATASPACE_SCALAR;

		int time_d_hid;
		int vorts_d_hid;
		int heats_d_hid;
		int *force_d_hid;
		int *friction_d_hid;
		int *nusselt_d_hid;
		int *position_d_hid;
		int *spring_d_hid;
		int *speed_d_hid;
};

#endif