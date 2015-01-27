#include <string>
#include <sys/file.h>

#include "log.h"
#include "vnchelper.h"

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		LOGE("Usage: %s <pipe path>", MODULE_NAME);
		return -1;
	}
	/* check for single instance */
	{
		std::string pidFile { argv[0] };
		pidFile += ".pid";
		int fd = open(pidFile.c_str(), O_CREAT | O_RDWR, 0666);
		int rc = flock(fd, LOCK_EX | LOCK_NB);
		if (rc)
		{
			/* either would block or error */
			LOGE("Already running, exiting now");
			return 0;
		}
	}
	LOGI("Starting %s, listening on %s", MODULE_NAME, argv[1]);
	if (VncHelper::getInstance().open(argv[1]) >= 0)
	{
		VncHelper::getInstance().run();
	}
	LOGI("Closing %s", MODULE_NAME);
	return 0;
}
