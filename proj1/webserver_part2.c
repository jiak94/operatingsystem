#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <linux/unistd.h>
#include <arpa/inet.h>


#define SERVER "webserver/1.0"
#define PROTOCOL "HTTP/1.0"
#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"

static volatile int threads_on_hold;

struct ListNode
{
	int x;
	struct node *next;
};

struct thread
{
	int id;
	pthread_t pthread;
	strcut thpool_* thpool_p;
};

struct thpool_
{
	thread** threads;
	volatile int num_alive;
	volatile int num_working;
	pthread_mutex_t lock;
	ListNode* list_p;
};



char *get_mime_type(char *name) {
		char *ext = strrchr(name, '.');
		if (!ext) return NULL;
		if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) return "text/html";
		if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
		if (strcmp(ext, ".gif") == 0) return "image/gif";
		if (strcmp(ext, ".png") == 0) return "image/png";
		if (strcmp(ext, ".css") == 0) return "text/css";
		if (strcmp(ext, ".au") == 0) return "audio/basic";
		if (strcmp(ext, ".wav") == 0) return "audio/wav";
		if (strcmp(ext, ".avi") == 0) return "video/x-msvideo";
		if (strcmp(ext, ".mpeg") == 0 || strcmp(ext, ".mpg") == 0) return "video/mpeg";
		if (strcmp(ext, ".mp3") == 0) return "audio/mpeg";
		return NULL;
}

void send_headers(FILE *f, int status, char *title, char *extra, char *mime, 
				int length, time_t date) {
		time_t now;
		char timebuf[128];

		fprintf(f, "%s %d %s\r\n", PROTOCOL, status, title);
		fprintf(f, "Server: %s\r\n", SERVER);
		now = time(NULL);
		strftime(timebuf, sizeof(timebuf), RFC1123FMT, gmtime(&now));
		fprintf(f, "Date: %s\r\n", timebuf);
		if (extra) fprintf(f, "%s\r\n", extra);
		if (mime) fprintf(f, "Content-Type: %s\r\n", mime);
		if (length >= 0) fprintf(f, "Content-Length: %d\r\n", length);
		if (date != -1) {
				strftime(timebuf, sizeof(timebuf), RFC1123FMT, gmtime(&date));
				fprintf(f, "Last-Modified: %s\r\n", timebuf);
		}
		fprintf(f, "Connection: close\r\n");
		fprintf(f, "\r\n");
}

void send_error(FILE *f, int status, char *title, char *extra, char *text) {
		send_headers(f, status, title, extra, "text/html", -1, -1);
		fprintf(f, "<HTML><HEAD><TITLE>%d %s</TITLE></HEAD>\r\n", status, title);
		fprintf(f, "<BODY><H4>%d %s</H4>\r\n", status, title);
		fprintf(f, "%s\r\n", text);
		fprintf(f, "</BODY></HTML>\r\n");
}

void send_file(FILE *f, char *path, struct stat *statbuf) {
		char data[4096];
		int n;

		FILE *file = fopen(path, "r");
		if (!file) {
				send_error(f, 403, "Forbidden", NULL, "Access denied.");
		} else {
				int length = S_ISREG(statbuf->st_mode) ? statbuf->st_size : -1;
				send_headers(f, 200, "OK", NULL, get_mime_type(path), length, statbuf->st_mtime);

				while ((n = fread(data, 1, sizeof(data), file)) > 0) fwrite(data, 1, n, f);
				fclose(file);
		}
}

int process(FILE *f) {
		char buf[4096];
		char *method;
		char *_path;
		char path[4096];
		char *protocol;
		struct stat statbuf;
		char pathbuf[4096];
		char cwd[1024];
		int len;
		struct sockaddr_in peer;
		int peer_len = sizeof(peer);

		if (!fgets(buf, sizeof(buf), f)) return -1;
		if(getpeername(fileno(f), (struct sockaddr*) &peer, &peer_len) != -1) {
				printf("[pid %d, tid %d] (from %s:%d) URL: %s", getpid(), syscall(__NR_gettid)-getpid(),inet_ntoa(peer.sin_addr), (int)ntohs(peer.sin_port), buf);
		} else {
				printf("[pid %d, tid %d] URL: %s", getpid(), syscall(__NR_gettid)-getpid(), buf);
		}

		method = strtok(buf, " ");
		_path = strtok(NULL, " ");
		protocol = strtok(NULL, "\r");
		if (!method || !_path || !protocol) return -1;

		getcwd(cwd, sizeof(cwd));
		sprintf(path, "%s%s", cwd, _path);

		fseek(f, 0, SEEK_CUR); // Force change of stream direction

		if (strcasecmp(method, "GET") != 0) {
				send_error(f, 501, "Not supported", NULL, "Method is not supported.");
				printf("[pid %d, tid %d] Reply: %s", getpid(), syscall(__NR_gettid)-getpid(), "Method is not supported.\n");
		} else if (stat(path, &statbuf) < 0) {
				send_error(f, 404, "Not Found", NULL, "File not found.");
				printf("[pid %d, tid %d] Reply: File not found - %s", getpid(), syscall(__NR_gettid)-getpid(), path);
		} else if (S_ISDIR(statbuf.st_mode)) {
				len = strlen(path);
				if (len == 0 || path[len - 1] != '/') {
						snprintf(pathbuf, sizeof(pathbuf), "Location: %s/", path);
						send_error(f, 302, "Found", pathbuf, "Directories must end with a slash.");
						printf("[pid %d, tid %d] Reply: %s", getpid(), syscall(__NR_gettid)-getpid(), "Directories mush end with a slash.\n");
				} else {
						snprintf(pathbuf, sizeof(pathbuf), "%s%sindex.html",cwd, path);
						if (stat(pathbuf, &statbuf) >= 0) {
								send_file(f, pathbuf, &statbuf);
								printf("[pid %d, tid %d] Reply: filesend %s\n", getpid(), syscall(__NR_gettid)-getpid(), pathbuf);
						} else {
								DIR *dir;
								struct dirent *de;

								send_headers(f, 200, "OK", NULL, "text/html", -1, statbuf.st_mtime);
								fprintf(f, "<HTML><HEAD><TITLE>Index of %s</TITLE></HEAD>\r\n<BODY>", path);
								fprintf(f, "<H4>Index of %s</H4>\r\n<PRE>\n", path);
								fprintf(f, "Name                             Last Modified              Size\r\n");
								fprintf(f, "<HR>\r\n");
								if (len > 1) fprintf(f, "<A HREF=\"..\">..</A>\r\n");

								dir = opendir(path);
								while ((de = readdir(dir)) != NULL) {
										char timebuf[32];
										struct tm *tm;

										strcpy(pathbuf, path);
										strcat(pathbuf, de->d_name);

										stat(pathbuf, &statbuf);
										tm = gmtime(&statbuf.st_mtime);
										strftime(timebuf, sizeof(timebuf), "%d-%b-%Y %H:%M:%S", tm);

										fprintf(f, "<A HREF=\"%s%s\">", de->d_name, S_ISDIR(statbuf.st_mode) ? "/" : "");
										fprintf(f, "%s%s", de->d_name, S_ISDIR(statbuf.st_mode) ? "/</A>" : "</A> ");
										if (strlen(de->d_name) < 32) fprintf(f, "%*s", 32 - strlen(de->d_name), "");
										if (S_ISDIR(statbuf.st_mode)) {
												fprintf(f, "%s\r\n", timebuf);
										} else {
												fprintf(f, "%s %10d\r\n", timebuf, statbuf.st_size);
										}
								}
								closedir(dir);

								fprintf(f, "</PRE>\r\n<HR>\r\n<ADDRESS>%s</ADDRESS>\r\n</BODY></HTML>\r\n", SERVER);
								printf("[pid %d, tid %d] Reply: SUCCEED\n", getpid(), syscall(__NR_gettid)-getpid());
						}
				}
		} else {
				send_file(f, path, &statbuf);
				printf("[pid %d, tid %d] Reply: filesend %s\n", getpid(), syscall(__NR_gettid)-getpid(), path);
		}

		return 0;
}



int listener(int port)
{
		int sock;
		struct sockaddr_in sin;
		struct sockaddr_in peer;
		int peer_len = sizeof(peer);

		sock = socket(AF_INET, SOCK_STREAM, 0);

		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = INADDR_ANY;
		sin.sin_port = htons(port);
		bind(sock, (struct sockaddr *) &sin, sizeof(sin));

		listen(sock, 5);
		printf("HTTP server listening on port %d\n", port);

		while (1) {
				int s;
				FILE *f;
				s = accept(sock, NULL, NULL);
				if (s < 0) break;
				if(getpeername(s, (struct sockaddr*) &peer, &peer_len) != -1) {
						printf("[pid %d, tid %d] Received a request from %s:%d\n", getpid(), syscall(__NR_gettid) - getpid(), inet_ntoa(peer.sin_addr), (int)ntohs(peer.sin_port));
				}
				f = fdopen(s, "a+");
				process(f);
				fclose(f);
		}

		close(sock);

}

struct thpool_* pool_init(int number) {
	threads_on_hold = 0;

	if (number < 0) {
		number = 0;
	}

	thpool_* thpool_p;

	thpool_p = (struct thpool_*)malloc(sizeof(struct thpool_));

	if (thpool_p == NULL)
	{
		return NULL;
	}

	thpool_p->num_alive = 0;
	thpool_p->num_working = 0;

	if (ListNode_init(thpool_p) == -1)
	{
		return NULL;
	}

	//Make thread in pool
	thpool_p->threads = (struct thread**)malloc(number * sizeof(struct thread));

	if (thpool_p->threads == NULL)
	{
		return NULL;
	}

	pthread_mutex_init(&(thpool_p->lock), NULL);

	int n;
	for (n = 0; n < number; n++) {
		thread_init(thpool_p, &thpool_p->threads[n], n);
	}

	while (thpool_p->num_alive != number){

	}

	return thpool_p;

}


void thread_init(thpool_* thpool_p, struct thread** thread_p, int id) {
	*thread_p = (struct thread*)malloc(sizeof(struct thread));

	if (thread_p == NULL)
	{
		exit(1);
	}

	(*thread_p)->thpool_p = thpool_p;
	(*thread_p)->id = id;

	pthread_create(&(*thread_p)->pthread, NULL, (void *)thread_do, (*thread_p));
	pthread_detach((*thread_p)->pthread);
}

static int ListNode_init(thpool_* thpool_p) {
	thpool_p->
}

int main(int argc, char *argv[]) {
		if(argc < 3 || atoi(argv[1]) < 2000 || atoi(argv[1]) > 50000)
		{
				fprintf(stderr, "./webserver PORT(2001 ~ 49999) #_of_threads\n");
				return 0;
		}
		int port = atoi(argv[1]);
		int N = atoi(argv[2]);

		listener(port);

		return 0;
}
