#include "stdio.h"
#include "unistd.h"
#include "string.h"
#include "malloc.h"
#include "libvvplot_api.h"
#include "core.h"

const char interp[] __attribute__((section(".interp"))) = "/lib64/ld-linux-x86-64.so.2";
static char buffer[4096];
Space *S = NULL;
hid_t fid;

inline
void get_args(int *argc, char **argv)
{
	FILE* argf = fopen("/proc/self/cmdline", "rb");
	size_t len = fread(buffer, 1, sizeof(buffer), argf);
	// for (int i=0; i<256; i++) printf("%c", buffer[i]?buffer[i]:'*'); printf("\n");
	for (char *p1=buffer, *p2=buffer; p1 < buffer+len; p1++)
	{
		if (*p1==0)
		{
			argv[(*argc)++] = p2;
			p2=p1+1;
		}
	}
	fclose(argf);
}

static hid_t DATASPACE_SCALAR = -1;
inline void attribute_write(hid_t hid, const char *name, double value)
{
	// if (value == 0) return;
	if (DATASPACE_SCALAR<0) DATASPACE_SCALAR = H5Screate(H5S_SCALAR);
	hid_t aid = H5Acreate2(hid, name, H5T_NATIVE_DOUBLE, DATASPACE_SCALAR, H5P_DEFAULT, H5P_DEFAULT);
	if (aid < 0) return;
	H5Awrite(aid, H5T_NATIVE_DOUBLE, &value);
	H5Aclose(aid);
}

extern "C" {
bool map_check(
	hid_t fid,
	const char *dsetname,
	double xmin,
	double xmax,
	double ymin,
	double ymax,
	double spacing)
{
	if (!H5Lexists(fid, dsetname, H5P_DEFAULT)) return false;
	hid_t dataset = H5Dopen2(fid, dsetname, H5P_DEFAULT);
	if (dataset < 0)
	{
		H5Epop(H5E_DEFAULT, H5Eget_num(H5E_DEFAULT)-1);
		H5Eprint2(H5E_DEFAULT, stderr);
		fprintf(stderr, "error: argument dataset: can't open dataset '%s'\n", dsetname);
		exit(-1);
	}

	double attr[5];
	attribute_read_double(dataset, "xmin", attr[0]);
	attribute_read_double(dataset, "xmax", attr[1]);
	attribute_read_double(dataset, "ymin", attr[2]);
	attribute_read_double(dataset, "ymax", attr[3]);
	attribute_read_double(dataset, "spacing", attr[4]);

	bool result = true;
	result &= attr[0] <= xmin;
	result &= attr[1] >= xmax;
	result &= attr[2] <= ymin;
	result &= attr[3] >= ymax;
	result &= attr[4] <= spacing;
	// fprintf(stderr, "%s %s %lf %lf %lf %lf %lf\n", filename, dsetname, xmin, xmax, ymin, ymax, spacing);
	// fprintf(stderr, "%lf %lf %lf %lf %lf\n", attr[0], attr[1], attr[2], attr[3], attr[4]);
	// fprintf(stderr, "returning %s\n", result?"true":"false");
	H5Dclose(dataset);
	return result;
}}

extern "C" {
int map_save(
	hid_t fid,
	const char *dsetname,
	const float* data, const hsize_t *dims,
	double xmin,
	double xmax,
	double ymin,
	double ymax,
	double spacing)
{
	// Create HDF file
	// H5Eset_auto2(H5E_DEFAULT, NULL, NULL);

	// Create dataspace and dataset
	hid_t file_dataspace = H5Screate_simple(2, dims, dims);
	if (file_dataspace < 0)
	{
		H5Epop(H5E_DEFAULT, H5Eget_num(H5E_DEFAULT)-1);
		H5Eprint2(H5E_DEFAULT, stderr);
		return 5;
	}
	if (H5Lexists(fid, dsetname, H5P_DEFAULT)) H5Ldelete(fid, dsetname, H5P_DEFAULT);
	// H5Fclose(fid);
	// return 0;

	hid_t file_dataset = H5Dcreate2(fid, dsetname, H5T_NATIVE_FLOAT, file_dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	if (file_dataset < 0)
	{
		H5Epop(H5E_DEFAULT, H5Eget_num(H5E_DEFAULT)-1);
		H5Eprint2(H5E_DEFAULT, stderr);
		return 5;
	}

	attribute_write(file_dataset, "xmin", xmin);
	attribute_write(file_dataset, "xmax", xmax);
	attribute_write(file_dataset, "ymin", ymin);
	attribute_write(file_dataset, "ymax", ymax);
	attribute_write(file_dataset, "spacing", spacing);
	
	herr_t err = H5Dwrite(file_dataset, H5T_NATIVE_FLOAT, H5S_ALL, file_dataspace, H5P_DEFAULT, data);
	if (err < 0)
	{
		H5Epop(H5E_DEFAULT, H5Eget_num(H5E_DEFAULT)-1);
		H5Eprint2(H5E_DEFAULT, stderr);
		return 5;
	}
	H5Dclose(file_dataset);
	H5Sclose(DATASPACE_SCALAR);
	H5Sclose(file_dataspace);
	// H5Fclose(fid);

	return 0;
}}

extern "C" {
int map_extract(hid_t fid, const char *dsetname)
{
	H5Eset_auto2(H5E_DEFAULT, NULL, NULL);
	hid_t dataset = H5Dopen2(fid, dsetname, H5P_DEFAULT);
	if (dataset < 0)
	{
		H5Epop(H5E_DEFAULT, H5Eget_num(H5E_DEFAULT)-1);
		H5Eprint2(H5E_DEFAULT, stderr);
		fprintf(stderr, "error: argument dataset: can't open dataset '%s'\n", dsetname);
		return 3;
	}

	double args[5];
	attribute_read_double(dataset, "xmin", args[0]);
	attribute_read_double(dataset, "xmax", args[1]);
	attribute_read_double(dataset, "ymin", args[2]);
	attribute_read_double(dataset, "ymax", args[3]);
	attribute_read_double(dataset, "spacing", args[4]);

	hid_t dataspace = H5Dget_space(dataset);
	if (dataspace < 0)
	{
		H5Epop(H5E_DEFAULT, H5Eget_num(H5E_DEFAULT)-1);
		H5Eprint2(H5E_DEFAULT, stderr);
		return 5;
	}
	hsize_t dims[2];
	H5Sget_simple_extent_dims(dataspace, dims, dims);
	float *mem = (float*)malloc(sizeof(float)*dims[0]*dims[1]);
	herr_t err = H5Dread(dataset, H5T_NATIVE_FLOAT, H5S_ALL, dataspace, H5P_DEFAULT, mem);
	if (err < 0)
	{
		H5Epop(H5E_DEFAULT, H5Eget_num(H5E_DEFAULT)-1);
		H5Eprint2(H5E_DEFAULT, stderr);
		return 5;
	}
	
	// fprintf(stdout, "%lf %lf %lf %lf %lf\n", xmin, xmax, ymin, ymax, spacing);
	// fwrite(args, sizeof(double), 5, stdout);
	for (int i=0; i<dims[0]; i++)
	for (int j=0; j<dims[1]; j++)
	{
		float x = args[0] + i*args[4];
		float y = args[2] + j*args[4];
		fwrite(&x, sizeof(float), 1, stdout);
		fwrite(&y, sizeof(float), 1, stdout);
		fwrite(mem+i*dims[1]+j, sizeof(float), 1, stdout);
	}
	fflush(stdout);
	// assert(c == dims[0]*dims[1]);
	// fwrite(dims, sizeof(hsize_t), 2, stdout);
	// fwrite(&xmin, sizeof(double), 1, stdout);

	free(mem);
	H5Sclose(dataspace);
	H5Dclose(dataset);
	// printf("%s\n", argv[0]);
	return 0;

}}

void print_version()
{
	fprintf(stderr, "libvvplot.so compiled with:\n");
	fprintf(stderr, " - libvvhd git_commit %s\n", Space().getGitInfo());
	unsigned ver[3];
	H5get_libversion(&ver[0], &ver[1], &ver[2]);
	fprintf(stderr, " - libhdf version %u.%u.%u\n", ver[0], ver[1], ver[2]);
	fflush(stderr);
}

void print_help()
{
	fprintf(stderr, "Usage: libvvplot.so {-h,-v,-M,-I} FILE DATASET [ARGS]\n");
	fprintf(stderr, "Options:\n");
	fprintf(stderr, " -h : show this message\n");
	fprintf(stderr, " -v : show version info\n");
	fprintf(stderr, " -M : extract a binary dataset from hdf file\n");
	fprintf(stderr, " -I : plot isolines on a dataset with constants in args\n");
	fflush(stderr);
}

int main()
{
	int argc = 0;
	char *argv[512];
	get_args(&argc, argv);

	char mode;
	     if (argc<2) { print_help(); _exit(1); }
	else if (!strcmp(argv[1], "-h")) { print_help(); _exit(0); }
	else if (!strcmp(argv[1], "-v")) { print_version(); _exit(0); }
	else if (argc < 4) { print_help(); _exit(1); }
	else if (!strcmp(argv[1], "-I") || !strcmp(argv[1], "-M")) {;}
	else {fprintf(stderr, "Bad option '%s'. See '-h' for help.\n", argv[1]); _exit(-1); }

	H5Eset_auto2(H5E_DEFAULT, NULL, NULL);
	fid = H5Fopen(argv[2], H5F_ACC_RDONLY, H5P_DEFAULT);
	if (fid < 0)
	{
		H5Epop(H5E_DEFAULT, H5Eget_num(H5E_DEFAULT)-1);
		H5Eprint2(H5E_DEFAULT, stderr);
		fprintf(stderr, "error: argument file: can't open file '%s'\n", argv[2]);
		return 2;
	}

	if (!strcmp(argv[1], "-M"))
	{
		map_extract(fid, argv[3]);
	}
	else if (!strcmp(argv[1], "-I"))
	{
		float vals[512];
		for (int i=4; i<argc; i++) { sscanf(argv[i], "%f", &vals[i-4]); }
		map_isoline(fid, argv[3], vals, argc-4);
	}
	H5Fclose(fid);

	_exit(0);
}//}

