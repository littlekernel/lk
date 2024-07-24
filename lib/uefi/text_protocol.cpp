#include "text_protocol.h"
#include <stdio.h>

EfiStatus output_string(struct EfiSimpleTextOutputProtocol *self,
                        char16_t *string) {
  char buffer[512];
  size_t i = 0;
  while (string[i]) {
    size_t j = 0;
    for (j = 0; j < sizeof(buffer) - 1 && string[i + j]; j++) {
      buffer[j] = string[i + j];
    }
    i += j;
    buffer[j] = 0;

    printf("%s", reinterpret_cast<const char *>(buffer));
  }
  return SUCCESS;
}

EfiSimpleTextOutputProtocol get_text_output_protocol() {
  EfiSimpleTextOutputProtocol console_out;
  console_out.output_string = output_string;
  return console_out;
}