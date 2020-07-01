/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <linux/vm_sockets.h>
#include <sys/un.h>

#include "battery_notifypkt.h"

#define TEST_PORT 14196
#define debug_local_flag 0

struct initial_pkt initpkt;
struct monitor_pkt monitorpkt;
struct monitor_pkt monitortemp;

int start_connection(struct sockaddr_vm sa_listen, int listen_fd, socklen_t socklen_client, int *m_acpidsock);

int client_fd = 0;
char base_path[100] = "/sys/class/power_supply/";

/*
 * get_battery_module_name : This function gets the battery module
 * name to be appended to the base path.
 */

void get_battery_module_name(char *buf)
{
	FILE *fp;
	char *cmd = "for i in `ls /sys/class/power_supply`; do if [ `cat /sys/class/power_supply/$i/type` = \"Battery\" ]; then echo $i;fi done;";
	if ((fp = popen(cmd, "r")) == NULL) {
		printf("Error opening pipe!\n");
		return;
	}

	while (fgets(buf, 50, fp) != NULL) {
		printf("Battery module name: %s", buf);
	}

	if(pclose(fp))  {
		printf("Command not found\n");
		return;
	}
	/* Delete the last new line character */
	buf[strlen(buf)-1] = 0;

	return;
}

/* read_sysfs_values: Function to read the filename sysfs value in battery
 * module.
 */

void read_sysfs_values(char *filename, void *buf, int len, int flag)
{
	char sysfs_path[120];

	snprintf(sysfs_path, 120, "%s%s", base_path, "/");
	snprintf(sysfs_path + strlen(sysfs_path), 120 - strlen(sysfs_path), "%s", filename);

	FILE *fp = fopen(sysfs_path, "r");
	if (!fp) {  /* validate file open for reading */
		fprintf (stderr, "Failed to open file for read.\n");
		return;
	}

	if (flag==0)
		fread(buf, len, 1, fp);
	else
    		fscanf (fp, "%d", (int*)buf);  /* read/validate value */
	fclose (fp);
	return;
}

/* read_store_values: Read and store all the battery related sysfs values
 * in global structure variables.
 */

void read_store_values()
{
	read_sysfs_values(MODEL_NAME, initpkt.model_name, sizeof(initpkt.model_name), 0);
	read_sysfs_values(SERIAL_NUMBER, initpkt.serial_number, sizeof(initpkt.serial_number), 0);
	read_sysfs_values(MANUFACTURER, initpkt.manufacturer, sizeof(initpkt.manufacturer), 0);
	read_sysfs_values(TECHNOLOGY, initpkt.technology, sizeof(initpkt.technology), 0);
	read_sysfs_values(TYPE, initpkt.type, sizeof(initpkt.type), 0);
	read_sysfs_values(PRESENT, initpkt.present, sizeof(initpkt.present), 0);
	read_sysfs_values(CAPACITY, &monitorpkt.capacity, sizeof(monitorpkt.capacity), 1);
	read_sysfs_values(CHARGE_FULL_DESIGN, &monitorpkt.charge_full_design, sizeof(monitorpkt.charge_full_design), 1);
	read_sysfs_values(HEALTH, monitorpkt.health, sizeof(monitorpkt.health), 0);
	read_sysfs_values(TEMP, &monitorpkt.temp, sizeof(monitorpkt.temp), 1);
	read_sysfs_values(CHARGE_NOW, &monitorpkt.charge_now, sizeof(monitorpkt.charge_now), 1);
	read_sysfs_values(TIME_TO_EMPTY_AVG, &monitorpkt.time_to_empty_avg, sizeof(monitorpkt.time_to_empty_avg), 1);
	read_sysfs_values(CHARGE_FULL, &monitorpkt.charge_full, sizeof(monitorpkt.charge_full), 1);
	read_sysfs_values(TIME_TO_FULL_NOW, &monitorpkt.time_to_full_now, sizeof(monitorpkt.time_to_full_now), 1);
	read_sysfs_values(VOLTAGE_NOW, &monitorpkt.voltage_now, sizeof(monitorpkt.voltage_now), 1);
	read_sysfs_values(CHARGE_TYPE, monitorpkt.charge_type, sizeof(monitorpkt.charge_type), 0);
	read_sysfs_values(CAPACITY_LEVEL, monitorpkt.capacity_level, sizeof(monitorpkt.capacity_level), 0);
	read_sysfs_values(STATUS, monitorpkt.status, sizeof(monitorpkt.status), 0);
#if debug_local_flag
	printf("In file %s: Read and store complete\n", __FILE__);
#endif
}

/* read_monitor_pkt: Reads the latest struct monitor_pkt variable values
 * compares the same with old values and returns 1 if changed and 0 otherwise.
 */

bool read_monitor_pkt(struct monitor_pkt *monitortemp)
{
#if debug_local_flag
	static int count = 0;
	count++;
#endif
	read_sysfs_values(CAPACITY, &monitortemp->capacity, sizeof(monitortemp->capacity), 1);
#if debug_local_flag
	if(count%10==0)
		monitortemp->capacity = 51;
#endif
	read_sysfs_values(CHARGE_FULL_DESIGN, &monitortemp->charge_full_design, sizeof(monitortemp->charge_full_design), 1);
	read_sysfs_values(HEALTH, monitortemp->health, sizeof(monitortemp->health), 0);
	read_sysfs_values(TEMP, &monitortemp->temp, sizeof(monitortemp->temp), 1);
	read_sysfs_values(CHARGE_NOW, &monitortemp->charge_now, sizeof(monitortemp->charge_now), 1);
	read_sysfs_values(TIME_TO_EMPTY_AVG, &monitortemp->time_to_empty_avg, sizeof(monitortemp->time_to_empty_avg), 1);
	read_sysfs_values(CHARGE_FULL, &monitortemp->charge_full, sizeof(monitortemp->charge_full), 1);
	read_sysfs_values(TIME_TO_FULL_NOW, &monitortemp->time_to_full_now, sizeof(monitortemp->time_to_full_now), 1);
	read_sysfs_values(VOLTAGE_NOW, &monitortemp->voltage_now, sizeof(monitortemp->voltage_now), 1);
	read_sysfs_values(CHARGE_TYPE, monitortemp->charge_type, sizeof(monitortemp->charge_type), 0);
	read_sysfs_values(CAPACITY_LEVEL, monitortemp->capacity_level, sizeof(monitortemp->capacity_level), 0);
	read_sysfs_values(STATUS, monitortemp->status, sizeof(monitortemp->status), 0);
/* FIXME Issue: Connection is lost if not sending for sometime. So as WA sending every
 * second as of now. Send only on value change
 */
#if 0
	/* Compare all the values of new structure with the already existing
	 * values. Return 1 if changed and 0 otherwise
	 */
	if (!(monitorpkt.capacity == monitortemp->capacity) || !(monitorpkt.charge_full_design == monitortemp->charge_full_design) || !(monitorpkt.temp == monitortemp->temp) || !(monitorpkt.charge_now == monitortemp->charge_now) || !(monitorpkt.time_to_empty_avg == monitortemp->time_to_empty_avg) || !(monitorpkt.charge_full == monitortemp->charge_full) || !(monitorpkt.time_to_full_now == monitortemp->time_to_full_now) || !(monitorpkt.voltage_now == monitortemp->voltage_now) || strcmp(monitorpkt.health,monitortemp->health) || strcmp(monitorpkt.charge_type, monitortemp->charge_type) || strcmp(monitorpkt.capacity_level, monitortemp->capacity_level) || strcmp(monitorpkt.status, monitortemp->status)) {
#if debug_local_flag
		printf("something changed\n");
#endif
		return 1;
	}
	else
		return 0;
#else
	return 1;
#endif
}

/* fill_header: Function to fill the header structure based on the
 * notification id passed and update the struct variable passed
 */

void fill_header (struct header *head, uint16_t id) {
	strcpy((char *)head->intelipc, INTELIPCID);
	head->notify_id = id;
	if (id == 1)
		head->length = sizeof(initpkt) + sizeof(monitorpkt);
	else if (id == 2)
		head->length = sizeof(monitorpkt);
}


#if debug_local_flag
int main()
#else
int send_pkt()
#endif
{
	char msgbuf[1024] = {0};
	struct header head;
	bool flag = 0;
	int return_value = 0;

#if debug_local_flag
	char battery_module_name[50];

	get_battery_module_name(battery_module_name);
	printf("Battery module name: %s\n",battery_module_name);

	/* Updating the base_path to point to the battery module */
	strcat(base_path, battery_module_name);

	printf("Starting the program\n");
#endif

	/* Read and store the battery sysfs values for the 1st time */
	read_store_values();
	fill_header(&head, 1);
	memcpy(msgbuf, (const unsigned char*)&head, sizeof(head));
	memcpy(msgbuf + sizeof(head), (const unsigned char*)&initpkt, sizeof(initpkt));
	memcpy(msgbuf + sizeof(head) + sizeof(initpkt), (const unsigned char*)&monitorpkt, sizeof(monitorpkt));
#if debug_local_flag
	printf("Sending initial values\n");
#else
	return_value = send(client_fd, msgbuf, sizeof(msgbuf), MSG_DONTWAIT);
	if (return_value == -1)
		goto out;
	memset(msgbuf, 0, sizeof(msgbuf));
#endif
#if debug_local_flag
	printf("Initial values sent\n");
	for(int i = 0; i < (sizeof(head) + sizeof(initpkt) + sizeof(monitorpkt)); i++)
		printf("%c", msgbuf[i]);
#endif
	while(1)
	{
		sleep(1);
		flag = 0;
		flag = read_monitor_pkt(&monitortemp);
		if (flag == 1) {
			monitorpkt = monitortemp;
			fill_header(&head, 2);
			memcpy(msgbuf, (const unsigned char*)&head, sizeof(head));
			memcpy(msgbuf + sizeof(head), (const unsigned char *)&monitortemp, sizeof(monitortemp));
#if debug_local_flag
			printf("Sending the changed values\n");
#else
			return_value = send(client_fd, msgbuf, sizeof(msgbuf), MSG_DONTWAIT);
			if (return_value == -1)
				goto out;
#endif
#if debug_local_flag
			for(int j = 0; j < sizeof(msgbuf); j++)
				printf("%c", msgbuf[j]);
#endif
		}
#if debug_local_flag
		else
			printf("nothing changed\n");
#endif
	}
	return 0;
out:
	return -1;
}

int start_connection(struct sockaddr_vm sa_listen, int listen_fd, socklen_t socklen_client, int *m_acpidsock) {
	int ret = 0;
	struct sockaddr_vm sa_client;
	struct sockaddr_un m_acpidsockaddr;
	fprintf(stderr, "Battery utility listening on cid(%d), port(%d)\n", sa_listen.svm_cid, sa_listen.svm_port);
	if (listen(listen_fd, 32) != 0) {
		fprintf(stderr, "listen failed\n");
		ret = -1;
		goto out;
	}

	client_fd = accept(listen_fd, (struct sockaddr*)&sa_client, &socklen_client);
	if(client_fd < 0) {
		fprintf(stderr, "accept failed\n");
		ret = -1;
		goto out;
	}
	fprintf(stderr, "Battery utility connected from guest(%d)\n", sa_client.svm_cid);

	/* Connect to acpid socket */
	*m_acpidsock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (*m_acpidsock < 0) {
		perror("new acpidsocket failed");
		ret = -2;
		goto out;
	}

	m_acpidsockaddr.sun_family = AF_UNIX;
	strcpy(m_acpidsockaddr.sun_path,"/var/run/acpid.socket");
	if(connect(*m_acpidsock, (struct sockaddr *)&m_acpidsockaddr, 108)<0)
	{
		/* can't connect */
		perror("connect acpidsocket failed");
		ret = -2;
		goto out;
	}
out:
	return ret;
}

#if !debug_local_flag
int main()
{
	int listen_fd = 0;
	int ret = 0;
	int return_value = 0;
	int m_acpidsock = 0;
	char battery_module_name[50];

	get_battery_module_name(battery_module_name);

	/* Updating the base_path to point to the battery module */
	strcat(base_path, battery_module_name);

	struct sockaddr_vm sa_listen = {
		.svm_family = AF_VSOCK,
		.svm_cid = VMADDR_CID_ANY,
		.svm_port = TEST_PORT,
	};
	socklen_t socklen_client = sizeof(struct sockaddr_vm);

	listen_fd = socket(AF_VSOCK, SOCK_STREAM, 0);
	if (listen_fd < 0) {
		fprintf(stderr, "socket init failed\n");
		ret = -1;
		goto out;
	}

	if (bind(listen_fd, (struct sockaddr*)&sa_listen, sizeof(sa_listen)) != 0) {
		perror("bind failed");
		ret = -1;
		goto out;
	}
start:
	ret = start_connection(sa_listen, listen_fd, socklen_client, &m_acpidsock);
	if (ret < 0)
		goto out;
	return_value = send_pkt();
	if (return_value == -1)
		goto start;
out:
	if(listen_fd >= 0)
	{
		printf("Closing listen_fd\n");
		close(listen_fd);
	}

	if(m_acpidsock >= 0)
	{
		printf("Closing acpisocket\n");
		close(m_acpidsock);
	}
	return ret;
}
#endif
