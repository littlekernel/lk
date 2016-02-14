#include <dev/accelerometer.h>

#include <err.h>

__WEAK uint acc_exists(void)
{
	return ACCELEROMETER_ABSENT;
}

__WEAK status_t acc_read_xyz(position_vector_t * pos_vector)
{
	return ERR_NOT_SUPPORTED;
}