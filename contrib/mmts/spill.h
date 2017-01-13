#ifndef __SPILL_H__
#define __SPILL_H__

void MtmSpillToFile(int fd, char const* data, size_t size);
void MtmCreateSpillDirectory(int node_id);
int  MtmCreateSpillFile(int node_id, int* file_id);
int  MtmOpenSpillFile(int node_id, int file_id);
void MtmReadSpillFile(int fd, char* data, size_t size);
void MtmCloseSpillFile(int fd);

#endif
