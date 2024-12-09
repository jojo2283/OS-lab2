#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "../include/cache.h"
#include "../include/my_shell.h"

const char* const BuiltinStr[] = {"help", "exit", "io-lat-read", "lin-reg", "both"};

int DanabolNumBuiltins(void) {
  return sizeof(BuiltinStr) / sizeof(char*);
}

int DanabolHelp(void) {
  printf("danabol by itmo\n");
  printf("Наберите название препарата (программы) и нажмите enter.\n");
  printf("Вот список встроенных  препаратов:\n");

  for (int i = 0; i < DanabolNumBuiltins(); i++) {
    printf("  %s\n", BuiltinStr[i]);
  }

  printf("Используйте команду man для получения информации по другим препаратам.\n");
  return 1;
}

int DanabolExit(void) {
  return 0;
}

int DanabolLaunch(char** args) {
  int status = 0;

  pid_t pid = fork();
  if (pid == 0) {
    if (execvp(args[0], args) == -1) {
      return -1;
    }
    pthread_exit(NULL);
  } else if (pid < 0) {
    return -1;
  } else {
    do {
      waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

char** DanabolSplitLine(char* line) {
  int bufsize = DANABOL_TOK_BUFSIZE;
  int position = 0;
  char** tokens = (char**)malloc(bufsize * sizeof(char*));

  if (tokens == NULL) {
    return NULL;
  }
  char* saveptr = NULL;

  char* token = strtok_r(line, DANABOL_TOK_DELIM, &saveptr);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += DANABOL_TOK_BUFSIZE;
      char** temp = (char**)realloc((void*)tokens, bufsize * sizeof(char*));
      if (!temp) {
        free((void*)tokens);
        return NULL;
      }
      tokens = temp;
    }

    token = strtok_r(NULL, DANABOL_TOK_DELIM, &saveptr);
  }
  tokens[position] = NULL;
  return tokens;
}

char* DanabolReadLine(void) {
  int bufsize = DANABOL_RL_BUFSIZE;
  int position = 0;
  char* buffer = malloc(sizeof(char) * bufsize);

  if (!buffer) {
    return NULL;
  }

  while (1) {
    int chr = getchar();

    if (chr == EOF || chr == '\n') {
      buffer[position] = '\0';
      return buffer;
    }
    buffer[position] = (char)chr;

    position++;
    if (position >= bufsize) {
      bufsize += DANABOL_RL_BUFSIZE;
      char* temp = realloc(buffer, bufsize);
      if (!temp) {
        free(buffer);
        return NULL;
      }
      buffer = temp;
    }
  }
}

long GetTimeInMicroseconds(void) {
  struct timeval timevalue;
  gettimeofday(&timevalue, NULL);
  return (timevalue.tv_sec * MICROSECONDS) + timevalue.tv_usec;
}

int DiskReadLatency(const char* file_path, long repetitions) {
 
  int target_file = lab2_open(file_path);
  if (target_file == -1) {
    return -1;
  }


  void* buffer = aligned_alloc(BLOCK_SIZE, BLOCK_SIZE);
  if (buffer == NULL) {
    lab2_close(target_file);
    return -1;
  }

  long start_time = 0;
  long end_time = 0;
  long total_time = 0;
  ssize_t bytes_read = 0;

  for (int i = 0; i < repetitions; i++) {

    lab2_lseek(target_file, 0, SEEK_SET);

    start_time = GetTimeInMicroseconds();

  
    while ((bytes_read = lab2_read(target_file, buffer, BLOCK_SIZE)) > 0) {
    }
    if (bytes_read == -1) {
      perror("Failed to read file");
      free(buffer);
      lab2_close(target_file);
      return -1;
    }

    // Останавливаем время
    end_time = GetTimeInMicroseconds();

    total_time += (end_time - start_time);
  }

  printf(
      "Total latency for %d-byte blocks: %.2f microseconds for %ld repetitions\n",
      BLOCK_SIZE,
      (double)total_time,
      repetitions
  );


  free(buffer);
  lab2_close(target_file);
  return 1;
}

int DanabolIoLatRead(char** argv) {
  int arg_count = 0;

  while (argv[arg_count] != NULL) {
    arg_count++;
  }

  if (arg_count < 3) {
    return 2;
  }

  char* endptr = NULL;


  const char* file_path = argv[1];
  long repetitions = strtol(argv[2], &endptr, NUMERAL_SYSTEM);


  return DiskReadLatency(file_path, repetitions);
}

int PerformLinearRegression(const char* input_file, const char* output_file, long repetitions) {
  // Открытие входного файла
  int in_fd = lab2_open(input_file);
  if (in_fd == -1) {
    perror("Failed to open input file");
    return -1;
  }

  // Открытие выходного файла
  int out_fd = lab2_open(output_file);
  if (out_fd == -1) {
    perror("Failed to open output file");
    lab2_close(in_fd);
    return -1;
  }

  // Буфер для чтения данных
  char line[LINE_SIZE];
  char* buffer = aligned_alloc(BLOCK_SIZE, BLOCK_SIZE);
  if (!buffer) {
    perror("Failed to allocate buffer");
    lab2_close(in_fd);
    lab2_close(out_fd);
    return -1;
  }

  double x_val = 0;
  double y_val = 0;
  double sum_x = 0;
  double sum_y = 0;
  double sum_xy = 0;
  double sum_x_squared = 0;
  int num = 0;
  ssize_t bytes_read = 0;

  // Чтение данных из входного файла построчно
  while ((bytes_read = lab2_read(in_fd, buffer, BLOCK_SIZE)) > 0) {
    char* line_ptr = buffer;
    while (line_ptr < buffer + bytes_read) {
      strncpy(line, line_ptr, LINE_SIZE - 1);
      line[LINE_SIZE - 1] = '\0';

      char* endptr = NULL;

      errno = 0;
      x_val = strtod(line, &endptr);
      if (errno != 0 || endptr == line || *endptr == '\n') {
        line_ptr += strlen(line) + 1;
        continue;
      }

      errno = 0;
      y_val = strtod(endptr, &endptr);
      if (errno != 0 || endptr == line || (*endptr != '\0' && *endptr != '\n')) {
        line_ptr += strlen(line) + 1;
        continue;
      }

      sum_x += x_val;
      sum_y += y_val;
      sum_xy += x_val * y_val;
      sum_x_squared += x_val * x_val;
      num++;

      line_ptr += strlen(line) + 1;
    }
  }

  if (bytes_read == -1) {
    perror("Failed to read input file");
    free(buffer);
    lab2_close(in_fd);
    lab2_close(out_fd);
    return -1;
  }

  // Выполнение линейной регрессии указанное количество раз
  for (int i = 0; i < repetitions; i++) {
    double slope = (num * sum_xy - sum_x * sum_y) / (num * sum_x_squared - sum_x * sum_x);
    double intercept = (sum_y - slope * sum_x) / num;

    char result[128];
    snprintf(result, sizeof(result), "Iteration %d: Slope = %.6f, Intercept = %.6f\n", i + 1, slope, intercept);

    if (lab2_write(out_fd, result, strlen(result)) == -1) {
      perror("Failed to write output file");
      free(buffer);
      lab2_close(in_fd);
      lab2_close(out_fd);
      return -1;
    }
  }

  // Освобождение ресурсов
  free(buffer);
  lab2_close(in_fd);
  lab2_close(out_fd);
  return 1;
}

int DanabolLinReg(char** argv) {
  int arg_count = 0;

  while (argv[arg_count] != NULL) {
    arg_count++;
  }

  if (arg_count < 4) {
    return 3;
  }

  char* endptr = NULL;

  long repetitions = strtol(argv[3], &endptr, NUMERAL_SYSTEM);

  const char* input_file = argv[1];
  const char* output_file = argv[2];

  if (repetitions <= 0) {
    return -1;
  }

  return PerformLinearRegression(input_file, output_file, repetitions);
}


typedef struct {
  const char* file_path;
  const char* input_file;
  const char* output_file;
  long repetitions;
} ThreadParams;

void* WrapperIoLatRead(void* arg) {
  char** argv = (char**)arg;
  DanabolIoLatRead(argv);
  return NULL;
}

void* WrapperLinReg(void* arg) {
  char** argv = (char**)arg;
  DanabolLinReg(argv);
  return NULL;
}

int DanabolBoth(char** argv) {
  pthread_t thread1 = 0;
  pthread_t thread2 = 0;

  int arg_count = 0;

  while (argv[arg_count] != NULL) {
    arg_count++;
  }

  if (arg_count < 5) {
    return 4;
  }


  const char* file_path = argv[1];
  const char* input_file = argv[2];
  const char* output_file = argv[3];

  char* func1_args[] = {(char*)"io-lat-read", (char*)file_path, argv[4], NULL};

  char* func2_args[] = {(char*)"lin-reg", (char*)input_file, (char*)output_file, argv[4], NULL};
  

  if (pthread_create(&thread1, NULL, WrapperIoLatRead, func1_args) != 0) {
    return -1;
  }

  if (pthread_create(&thread2, NULL, WrapperLinReg, func2_args) != 0) {
    return -1;
  }

  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL);

  return 1;
}

int DanabolExecute(char** args) {
  if (args[0] == NULL) {
    return 1;
  }

  if (strcmp(args[0], "help") == 0) {
    return DanabolHelp();
  }
  if (strcmp(args[0], "exit") == 0) {
    return DanabolExit();
  }
  if (strcmp(args[0], "io-lat-read") == 0) {
    return DanabolIoLatRead(args);
  }
  if (strcmp(args[0], "lin-reg") == 0) {
    return DanabolLinReg(args);
  }
  if (strcmp(args[0], "both") == 0) {
    return DanabolBoth(args);
  }
  return DanabolLaunch(args);
}

int DanabolLoop(void) {
  int status = 1;

  do {
    printf("☀ ");
    char* line = DanabolReadLine();

    char** args = DanabolSplitLine(line);

    time_t start = time(NULL);
    status = DanabolExecute(args);
    time_t end = time(NULL);

    free(line);
    free((void*)args);

    if (status == -1) {
      perror("Ошибка во время выполнения");
      continue;
    }
    if (status == 2) {
      (void)fprintf(stderr, "Usage: %s <file_path> <repetitions>\n", "io-lat-read");
      continue;
    }
    if (status == 3) {
      (void)fprintf(stderr, "Usage: %s <file_path> <repetitions>\n", "lin-reg");
      continue;
    }

    double time_taken = difftime(end, start);
    printf("Время выполнения упражнения: %.2f секунд\n", time_taken);
  } while (status);

  return EXIT_SUCCESS;
}
