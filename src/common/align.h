/*
 * align.h
 *
 *  Created on: Mar 10, 2016
 *      Author: duping
 */

#ifndef ALIGN_H_
#define ALIGN_H_

extern int default_align;
extern int default_disk_align;

#define ALIGN_DEFAULT(p) ALIGN_BYTES(p, default_align)
#define ALIGN_DISK_DEFAULT(p) ALIGN_BYTES(p, default_disk_align)

#define ALIGN_BYTES(p, n) (((p) + (n) - 1) & ~((n) - 1))
#define ALIGN_EXP(p, n) (((p) + 1 << (n) - 1) >> (n) << (n))

#endif /* ALIGN_H_ */
