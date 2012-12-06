#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#if !defined(__MINGW64__) && !defined(__MINGW32__)
#include <sys/wait.h>
#endif
#include <string.h>

#include "autoOpen.h"

// For Windows, we use the CRT's _popen and _pclose primitives.
// _popen does not reveal the PID of the invoked process, so we just
// use seqFile->pid as a boolean flag.
#if defined(__MINGW64__) || defined(__MINGW32__)
static const char const* decompressors[] = {"","pigz", "gunzip", "pbunzip2", "bunzip2", NULL};

AutoFile* openFileAuto(char*filename)
{
	AutoFile* seqFile = calloc(1, sizeof(AutoFile));
  	int i;

	if (strcmp(filename, "-")==0)
		  exitErrorf(EXIT_FAILURE, false, "Cannot read from stdin in auto mode\n");

	for (i=0; decompressors[i] ; i++) {
		if (strlen(decompressors[i])==0) {
			seqFile->file = fopen(filename, "r");
			seqFile->pid = 0;
			seqFile->decompressor = "Raw read";
		} else {
			char *cmd = calloc(strlen(decompressors[i]) + strlen(filename) + 8, sizeof(char));
			sprintf(cmd, "%s -c -d %s", decompressors[i], filename);
			seqFile->file = _popen(cmd, "r");
			seqFile->decompressor = decompressors[i];
			seqFile->pid = 1;
		}

		if (!seqFile->file)
			continue;

		int c = fgetc(seqFile->file);
		if (c=='>' || c=='@') {
			// Ok, looks like FASTA or FASTQ
            ungetc(c, seqFile->file);
            seqFile->first_char = c;
			return seqFile;
		} else {
			if (seqFile->pid)
				_pclose(seqFile->file);
			else
				fclose(seqFile->file);
		}
	}
	//printf("Unable to determine file type\n");
	return NULL;
}

void closeFileAuto(AutoFile* seqFile)
{
	if (!seqFile)
		return;

	if (seqFile->pid)
		_pclose(seqFile->file);
	else
		fclose(seqFile->file);
}
#else
// Implementation of "popen" that ignores stderr
static FILE* popenNoStderr(const char *exe, const char *const argv[], int* retPid)
{
	int out[2];
	int pid;
	int rc;

	rc = pipe(out);
	if (rc<0)
		goto error_out;

	pid = fork();
	if (pid > 0) { // parent
		close(out[1]);
                *retPid = pid;
                return fdopen(out[0], "r");
	} else if (pid == 0) { // child
		close(out[0]);
		close(1);
		dup(out[1]);

		close(0);   // Don't let child inherit stdin, nor stderr
		close(2);

		execvp(exe, (char**)argv);
		exit(1);
	} else
		goto error_fork;

	return NULL;

error_fork:
	close(out[0]);
	close(out[1]);
error_out:
	return NULL;
}

static int pcloseNoStderr(int pid, FILE* out)
{
	int rc, status;
        fclose(out);
	rc = waitpid(pid, &status, 0);
	return status;
}


static const char const* decompressors[] = {"","pigz", "gunzip", "pbunzip2", "bunzip2", NULL};

AutoFile* openFileAuto(char*filename)
{
	AutoFile* seqFile = calloc(1, sizeof(AutoFile));
  	int i;

	if (strcmp(filename, "-")==0)
		  exitErrorf(EXIT_FAILURE, false, "Cannot read from stdin in auto mode\n");

	for (i=0; decompressors[i] ; i++) {
		if (strlen(decompressors[i])==0) {
			seqFile->file = fopen(filename, "r");
			seqFile->pid = 0;
			seqFile->decompressor = "Raw read";
		} else {
			//printf("Trying : %s\n", decompressors[i]);
			char const* args[] = {decompressors[i], "-c", "-d", filename, NULL};
			seqFile->file = popenNoStderr(args[0], args, &(seqFile->pid));
			seqFile->decompressor = decompressors[i];
		}

		if (!seqFile->file)
			continue;

		int c = fgetc(seqFile->file);
		if (c=='>' || c=='@') {
			// Ok, looks like FASTA or FASTQ
                	ungetc(c, seqFile->file);
                        seqFile->first_char = c;
			return seqFile;
		} else {
			if (seqFile->pid)
				pcloseNoStderr(seqFile->pid, seqFile->file);
			else
				fclose(seqFile->file);
		}
	}
	//printf("Unable to determine file type\n");
	return NULL;
}

void closeFileAuto(AutoFile* seqFile)
{
	if (!seqFile)
		return;

	if (seqFile->pid)
		pcloseNoStderr(seqFile->pid, seqFile->file);
	else
		fclose(seqFile->file);
}
#endif
