/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2015, 2016 Christian Egli, Swiss Library for the Blind, Visually Impaired and Print Disabled

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <config.h>
#include <assert.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#include "louis.h"
#include "error.h"
#include "progname.h"
#else
#include "..\liblouis\louis.h"
#include "gnulib\error.h"
#define program_name "lou_checkyaml.exe"
#endif /* _WIN32 */

#include "version-etc.h"
#include "brl_checks.h"

static const struct option longopts[] =
{
  { "help", no_argument, NULL, 'h' },
  { "version", no_argument, NULL, 'v' },
  { NULL, 0, NULL, 0 }
};

const char version_etc_copyright[] =
  "Copyright %s %d Swiss Library for the Blind, Visually Impaired and Print Disabled";

#define AUTHORS "Christian Egli"

static void
print_help (void)
{
  printf ("\
Usage: %s YAML_TEST_FILE\n", program_name);
  
  fputs ("\
Run the tests defined in the YAML_TEST_FILE. Return 0 if all tests pass\n\
or 1 if any of the tests fail. The details of failing tests are printed\n\
to stderr.\n\n", stdout);

  fputs ("\
  -h, --help          display this help and exit\n\
  -v, --version       display version information and exit\n", stdout);

  printf ("\n");
  printf ("Report bugs to %s.\n", PACKAGE_BUGREPORT);

#ifdef PACKAGE_PACKAGER_BUG_REPORTS
  printf ("Report %s bugs to: %s\n", PACKAGE_PACKAGER, PACKAGE_PACKAGER_BUG_REPORTS);
#endif
#ifdef PACKAGE_URL
  printf ("%s home page: <%s>\n", PACKAGE_NAME, PACKAGE_URL);
#endif
}

#define EXIT_SKIPPED 77

#ifdef HAVE_LIBYAML
#include <yaml.h>

const char* event_names[] = {"YAML_NO_EVENT", "YAML_STREAM_START_EVENT", "YAML_STREAM_END_EVENT",
			     "YAML_DOCUMENT_START_EVENT", "YAML_DOCUMENT_END_EVENT",
			     "YAML_ALIAS_EVENT", "YAML_SCALAR_EVENT",
			     "YAML_SEQUENCE_START_EVENT", "YAML_SEQUENCE_END_EVENT",
			     "YAML_MAPPING_START_EVENT", "YAML_MAPPING_END_EVENT"};
const char* encoding_names[] = {"YAML_ANY_ENCODING", "YAML_UTF8_ENCODING",
				"YAML_UTF16LE_ENCODING", "YAML_UTF16BE_ENCODING"};

yaml_parser_t parser;
yaml_event_t event;

char *file_name;
int translation_mode = 0;

int errors = 0;
int count = 0;

static char** emph_classes = NULL;

void
simple_error (const char *msg, yaml_parser_t *parser, yaml_event_t *event) {
  error_at_line(EXIT_FAILURE, 0, file_name, event->start_mark.line ? event->start_mark.line + 1 : parser->problem_mark.line + 1, "%s", msg);
}

void
yaml_parse_error (yaml_parser_t *parser) {
  error_at_line(EXIT_FAILURE, 0, file_name, parser->problem_mark.line + 1, "%s", parser->problem);
}

void
yaml_error (yaml_event_type_t expected, yaml_event_t *event) {
  error_at_line(EXIT_FAILURE, 0, file_name, event->start_mark.line + 1,
		"Expected %s (actual %s)",
		event_names[expected], event_names[event->type]);
}

void
read_tables (yaml_parser_t *parser, char *tables_list) {
  yaml_event_t event;
  if (!yaml_parser_parse(parser, &event) ||
      (event.type != YAML_SCALAR_EVENT) ||
      strcmp(event.data.scalar.value, "tables")) {
    simple_error("tables expected", parser, &event);
  }

  yaml_event_delete(&event);

  if (!yaml_parser_parse(parser, &event) ||
      (event.type != YAML_SEQUENCE_START_EVENT))
    yaml_error(YAML_SEQUENCE_START_EVENT, &event);

  yaml_event_delete(&event);

  int done = 0;
  char *p = tables_list;
  while (!done) {
    if (!yaml_parser_parse(parser, &event)) {
      yaml_parse_error(parser);
    }
    if (event.type == YAML_SEQUENCE_END_EVENT) {
      done = 1;
    } else if (event.type == YAML_SCALAR_EVENT ) {
      if (tables_list != p) strcat(p++, ",");
      strcat(p, event.data.scalar.value);
      p += event.data.scalar.length;
    }
    yaml_event_delete(&event);
  }
  emph_classes = getEmphClasses(tables_list); // get declared emphasis classes
}

void
read_flags (yaml_parser_t *parser, int *direction, int *hyphenation) {
  yaml_event_t event;
  int parse_error = 1;

  *direction = 0;
  *hyphenation = 0;

  if (!yaml_parser_parse(parser, &event) ||
      (event.type != YAML_MAPPING_START_EVENT))
    yaml_error(YAML_MAPPING_START_EVENT, &event);

  yaml_event_delete(&event);

  while ((parse_error = yaml_parser_parse(parser, &event)) &&
	 (event.type == YAML_SCALAR_EVENT)) {
    if (!strcmp(event.data.scalar.value, "testmode")) {
      yaml_event_delete(&event);
      if (!yaml_parser_parse(parser, &event) ||
	  (event.type != YAML_SCALAR_EVENT))
	yaml_error(YAML_SCALAR_EVENT, &event);
      if (!strcmp(event.data.scalar.value, "forward")) {
	*direction = 0;
      } else if (!strcmp(event.data.scalar.value, "backward")) {
	*direction = 1;
      } else if (!strcmp(event.data.scalar.value, "hyphenate")) {
	*hyphenation = 1;
      } else {
	error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
		      "Testmode '%s' not supported\n", event.data.scalar.value);
      }
    } else {
      error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
		    "Flag '%s' not supported\n", event.data.scalar.value);
    }
  }
  if (!parse_error)
    yaml_parse_error(parser);
  if (event.type != YAML_MAPPING_END_EVENT)
    yaml_error(YAML_MAPPING_END_EVENT, &event);
  yaml_event_delete(&event);
}

int
read_xfail (yaml_parser_t *parser) {
  yaml_event_t event;
  /* assume xfail true if there is an xfail key */
  int xfail = 1;
  if (!yaml_parser_parse(parser, &event) ||
      (event.type != YAML_SCALAR_EVENT))
    yaml_error(YAML_SCALAR_EVENT, &event);
  if (!strcmp(event.data.scalar.value, "false") ||
      !strcmp(event.data.scalar.value, "off"))
    xfail = 0;
  yaml_event_delete(&event);
  return xfail;
}

translationModes
read_mode (yaml_parser_t *parser) {
  yaml_event_t event;
  translationModes mode = 0;
  int parse_error = 1;

  if (!yaml_parser_parse(parser, &event) ||
      (event.type != YAML_SEQUENCE_START_EVENT))
    yaml_error(YAML_SEQUENCE_START_EVENT, &event);
  yaml_event_delete(&event);

  while ((parse_error = yaml_parser_parse(parser, &event)) &&
	 (event.type == YAML_SCALAR_EVENT)) {
    if (!strcmp(event.data.scalar.value, "noContractions")) {
      mode |= noContractions;
    } else if (!strcmp(event.data.scalar.value, "compbrlAtCursor")) {
      mode |= compbrlAtCursor;
    } else if (!strcmp(event.data.scalar.value, "dotsIO")) {
      mode |= dotsIO;
    } else if (!strcmp(event.data.scalar.value, "comp8Dots")) {
      mode |= comp8Dots;
    } else if (!strcmp(event.data.scalar.value, "pass1Only")) {
      mode |= pass1Only;
    } else if (!strcmp(event.data.scalar.value, "compbrlLeftCursor")) {
      mode |= compbrlLeftCursor;
    } else if (!strcmp(event.data.scalar.value, "otherTrans")) {
      mode |= otherTrans;
    } else if (!strcmp(event.data.scalar.value, "ucBrl")) {
      mode |= ucBrl;
    } else {
      error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
		    "Mode '%s' not supported\n", event.data.scalar.value);
    }
    yaml_event_delete(&event);
  }
  if (!parse_error)
    yaml_parse_error(parser);
  if (event.type != YAML_SEQUENCE_END_EVENT)
    yaml_error(YAML_SEQUENCE_END_EVENT, &event);
  yaml_event_delete(&event);
  return mode;
}

int *
read_cursorPos (yaml_parser_t *parser, int len) {
  int *pos = malloc(sizeof(int) * len);
  int i = 0;
  yaml_event_t event;
  int parse_error = 1;
  char *tail;

  if (!yaml_parser_parse(parser, &event) ||
      (event.type != YAML_SEQUENCE_START_EVENT))
    yaml_error(YAML_SEQUENCE_START_EVENT, &event);
  yaml_event_delete(&event);

  while ((parse_error = yaml_parser_parse(parser, &event)) &&
	 (event.type == YAML_SCALAR_EVENT)) {
    pos[i++] = strtol(event.data.scalar.value, &tail, 0);
    if (!pos && !strcmp(event.data.scalar.value, tail))
      error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
		    "Not a valid cursor position '%s'. Must be a number\n",
		    event.data.scalar.value);
    yaml_event_delete(&event);
  }
  if (i != len)
    error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
		  "Too many or too few cursor positions (%i) for word of length %i\n", i, len);
  if (!parse_error)
    yaml_parse_error(parser);
  if (event.type != YAML_SEQUENCE_END_EVENT)
    yaml_error(YAML_SEQUENCE_END_EVENT, &event);
  yaml_event_delete(&event);
  return pos;
}

void
read_typeform_string(yaml_parser_t *parser, formtype* typeform, typeforms kind, int len) {
  yaml_event_t event;
  int typeform_len;

  if (!yaml_parser_parse(parser, &event) || (event.type != YAML_SCALAR_EVENT))
    yaml_error(YAML_SCALAR_EVENT, &event);
  typeform_len = strlen(event.data.scalar.value);
  if (typeform_len != len)
    error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
		  "Too many or too typeforms (%i) for word of length %i\n", typeform_len, len);
  update_typeform(event.data.scalar.value, typeform, kind);
  yaml_event_delete(&event);
}

formtype*
read_typeforms (yaml_parser_t *parser, int len) {
  yaml_event_t event;
  formtype *typeform = calloc(len, sizeof(formtype));
  int parse_error = 1;

  if (!yaml_parser_parse(parser, &event) ||
      (event.type != YAML_MAPPING_START_EVENT))
    yaml_error(YAML_MAPPING_START_EVENT, &event);
  yaml_event_delete(&event);

  while ((parse_error = yaml_parser_parse(parser, &event)) &&
	 (event.type == YAML_SCALAR_EVENT)) {
    if (strcmp(event.data.scalar.value, "computer_braille") == 0) {
      yaml_event_delete(&event);
      read_typeform_string(parser, typeform, computer_braille, len);
    } else if (strcmp(event.data.scalar.value, "no_translate") == 0) {
      yaml_event_delete(&event);
      read_typeform_string(parser, typeform, no_translate, len);
    } else if (strcmp(event.data.scalar.value, "no_contract") == 0) {
      yaml_event_delete(&event);
      read_typeform_string(parser, typeform, no_contract, len);
    } else {
      int i;
      typeforms kind = plain_text;
      for (i = 0; emph_classes[i]; i++) {
        if (strcmp(event.data.scalar.value, emph_classes[i]) == 0) {
          yaml_event_delete(&event);
          kind = italic << i;
          if (kind > emph_10)
            error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
                          "Typeform '%s' was not declared\n", event.data.scalar.value);
          read_typeform_string(parser, typeform, kind, len);
          break;
        }
      }
    }
  }
  if (!parse_error)
    yaml_parse_error(parser);

  if (event.type != YAML_MAPPING_END_EVENT)
    yaml_error(YAML_MAPPING_END_EVENT, &event);
  yaml_event_delete(&event);
  return typeform;
}

void
read_options (yaml_parser_t *parser, int len,
	      int *xfail, translationModes *mode,
	      formtype **typeform, int **cursorPos) {
  yaml_event_t event;
  char *option_name;
  int parse_error = 1;

  *mode = 0;
  *xfail = 0;
  *typeform = NULL;
  *cursorPos = NULL;

  while ((parse_error = yaml_parser_parse(parser, &event)) &&
	 (event.type == YAML_SCALAR_EVENT)) {
    option_name = strndup(event.data.scalar.value, event.data.scalar.length);

    if (!strcmp(option_name, "xfail")) {
      yaml_event_delete(&event);
      *xfail = read_xfail(parser);
    } else if (!strcmp(option_name, "mode")) {
      yaml_event_delete(&event);
      *mode = read_mode(parser);
    } else if (!strcmp(option_name, "typeform")) {
      yaml_event_delete(&event);
      *typeform = read_typeforms(parser, len);
    } else if (!strcmp(option_name, "cursorPos")) {
      yaml_event_delete(&event);
      *cursorPos = read_cursorPos(parser, len);
    } else {
      error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
		    "Unsupported option %s", option_name);
    }
    free(option_name);
  }
  if (!parse_error)
    yaml_parse_error(parser);
  if (event.type != YAML_MAPPING_END_EVENT)
    yaml_error(YAML_MAPPING_END_EVENT, &event);
  yaml_event_delete(&event);
}

/* see http://stackoverflow.com/questions/5117393/utf-8-strings-length-in-linux-c */
int
my_strlen_utf8_c(char *s) {
   int i = 0, j = 0;
   while (s[i]) {
     if ((s[i] & 0xc0) != 0x80) j++;
     i++;
   }
   return j;
}

void
read_test(yaml_parser_t *parser, char *tables_list, int direction, int hyphenation) {
  yaml_event_t event;
  char *description = NULL;
  char *word;
  char *translation;
  int xfail = 0;
  translationModes mode = 0;
  formtype *typeform = NULL;
  int *cursorPos = NULL;

  if (!yaml_parser_parse(parser, &event) ||
      (event.type != YAML_SCALAR_EVENT))
    simple_error("Word expected", parser, &event);

  word = strndup(event.data.scalar.value, event.data.scalar.length);
  yaml_event_delete(&event);

  if (!yaml_parser_parse(parser, &event) ||
      (event.type != YAML_SCALAR_EVENT))
    simple_error("Translation expected", parser, &event);

  translation = strndup(event.data.scalar.value, event.data.scalar.length);
  yaml_event_delete(&event);

  if (!yaml_parser_parse(parser, &event))
    yaml_parse_error(parser);

  /* Handle an optional description */
  if (event.type == YAML_SCALAR_EVENT) {
    description = word;
    word = translation;
    translation = strndup(event.data.scalar.value, event.data.scalar.length);
    yaml_event_delete(&event);

    if (!yaml_parser_parse(parser, &event))
      yaml_parse_error(parser);
  }

  if (event.type == YAML_MAPPING_START_EVENT) {
    yaml_event_delete(&event);
    read_options(parser, my_strlen_utf8_c(word), &xfail, &mode, &typeform, &cursorPos);

    if (!yaml_parser_parse(parser, &event) ||
	(event.type != YAML_SEQUENCE_END_EVENT))
      yaml_error(YAML_SEQUENCE_END_EVENT, &event);
  } else if (event.type != YAML_SEQUENCE_END_EVENT) {
    error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
		  "Expected %s or %s (actual %s)",
		  event_names[YAML_MAPPING_START_EVENT],
		  event_names[YAML_SEQUENCE_END_EVENT],
		  event_names[event.type]);
  }

  if (cursorPos) {
    if (xfail != check_cursor_pos(tables_list, word, cursorPos)) {
      if (description)
	fprintf(stderr, "%s\n", description);
      error_at_line(0, 0, file_name, event.start_mark.line + 1,
		    (xfail ? "Unexpected Pass" :"Failure"));
      errors++;
    }
  } else if (hyphenation) {
    if (xfail != check_hyphenation(tables_list, word, translation)) {
      if (description)
	fprintf(stderr, "%s\n", description);
      error_at_line(0, 0, file_name, event.start_mark.line + 1,
		    (xfail ? "Unexpected Pass" :"Failure"));
      errors++;
    }
  } else {
    if (xfail != check_with_mode(tables_list, word, typeform,
				 translation, translation_mode, direction, !xfail)) {
      if (description)
	fprintf(stderr, "%s\n", description);
      error_at_line(0, 0, file_name, event.start_mark.line + 1,
		    (xfail ? "Unexpected Pass" :"Failure"));
      errors++;
    }
  }
  yaml_event_delete(&event);
  count++;
  free(description);
  free(word);
  free(translation);
  free(typeform);
  free(cursorPos);
}

void
read_tests(yaml_parser_t *parser, char *tables_list, int direction, int hyphenation) {
  yaml_event_t event;
  if (!yaml_parser_parse(parser, &event) ||
      (event.type != YAML_SEQUENCE_START_EVENT))
    yaml_error(YAML_SEQUENCE_START_EVENT, &event);

  yaml_event_delete(&event);
  
  int done = 0;
  while (!done) {
    if (!yaml_parser_parse(parser, &event)) {
      yaml_parse_error(parser);
    }
    if (event.type == YAML_SEQUENCE_END_EVENT) {
      done = 1;
      yaml_event_delete(&event);
    } else if (event.type == YAML_SEQUENCE_START_EVENT) {
      yaml_event_delete(&event);
      read_test(parser, tables_list, direction, hyphenation);
    } else {
      error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
		    "Expected %s or %s (actual %s)",
		    event_names[YAML_SEQUENCE_END_EVENT],
		    event_names[YAML_SEQUENCE_START_EVENT],
		    event_names[event.type]);
    }
  }
}

#endif // HAVE_LIBYAML

int
main(int argc, char *argv[]) {
  int optc;

#ifndef _WIN32
  set_program_name(argv[0]);
#endif

  while ((optc = getopt_long (argc, argv, "hv", longopts, NULL)) != -1)
    switch (optc) {
      /* --help and --version exit immediately, per GNU coding standards.  */
    case 'v':
      version_etc (stdout, program_name, PACKAGE_NAME, VERSION, AUTHORS, (char *) NULL);
      exit (EXIT_SUCCESS);
      break;
    case 'h':
      print_help ();
      exit (EXIT_SUCCESS);
      break;
    default:
      fprintf (stderr, "Try `%s --help' for more information.\n",
	       program_name);
      exit (EXIT_FAILURE);
      break;
    }

  if (optind != argc - 1) {
    /* Print error message and exit.  */
    if (optind < argc - 1)
      fprintf (stderr, "%s: extra operand: %s\n", program_name, argv[optind + 1]);
    else
      fprintf (stderr, "%s: no YAML test file specified\n", program_name);
    
    fprintf (stderr, "Try `%s --help' for more information.\n", program_name);
    exit (EXIT_FAILURE);
  }

#ifndef HAVE_LIBYAML
  fprintf(stderr, "Skipping tests for %s as libyaml was not found\n", argv[1]);
  return EXIT_SKIPPED;

#else

  FILE *file;
  yaml_parser_t parser;
  yaml_event_t event;

  int direction = 0;
  int hyphenation = 0;

  file = fopen(argv[1], "rb");
  assert(file);

  file_name = argv[1];

  assert(yaml_parser_initialize(&parser));

  yaml_parser_set_input_file(&parser, file);

  if (!yaml_parser_parse(&parser, &event) ||
      (event.type != YAML_STREAM_START_EVENT)) {
    yaml_error(YAML_STREAM_START_EVENT, &event);
  }

  if (event.data.stream_start.encoding != YAML_UTF8_ENCODING)
    error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
		  "UTF-8 encoding expected (actual %s)",
		  encoding_names[event.data.stream_start.encoding]);
  yaml_event_delete(&event);

  if (!yaml_parser_parse(&parser, &event) ||
      (event.type != YAML_DOCUMENT_START_EVENT)) {
    yaml_error(YAML_DOCUMENT_START_EVENT, &event);
  }
  yaml_event_delete(&event);

  if (!yaml_parser_parse(&parser, &event) ||
      (event.type != YAML_MAPPING_START_EVENT)) {
    yaml_error(YAML_MAPPING_START_EVENT, &event);
  }
  yaml_event_delete(&event);

  char *tables_list = malloc(sizeof(char) * MAXSTRING);
  tables_list[0] = '\0';
  read_tables(&parser, tables_list);

  if (!lou_getTable(tables_list)) {
    error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
		  "Table %s not valid", tables_list);
  }
  if (!yaml_parser_parse(&parser, &event) ||
      (event.type != YAML_SCALAR_EVENT)) {
    yaml_error(YAML_SCALAR_EVENT, &event);
  }

  if (!strcmp(event.data.scalar.value, "flags")) {
    yaml_event_delete(&event);
    read_flags(&parser, &direction, &hyphenation);

    if (!yaml_parser_parse(&parser, &event) ||
	(event.type != YAML_SCALAR_EVENT) ||
	strcmp(event.data.scalar.value, "tests")) {
      simple_error("tests expected", &parser, &event);
    }
    yaml_event_delete(&event);
    read_tests(&parser, tables_list, direction, hyphenation);

  } else if (!strcmp(event.data.scalar.value, "tests")) {
    yaml_event_delete(&event);
    read_tests(&parser, tables_list, direction, hyphenation);
  } else {
    simple_error("flags or tests expected", &parser, &event);
  }

  if (!yaml_parser_parse(&parser, &event) ||
      (event.type != YAML_MAPPING_END_EVENT)) {
    yaml_error(YAML_MAPPING_END_EVENT, &event);
  }
  yaml_event_delete(&event);

  if (!yaml_parser_parse(&parser, &event) ||
      (event.type != YAML_DOCUMENT_END_EVENT)) {
    yaml_error(YAML_DOCUMENT_END_EVENT, &event);
  }
  yaml_event_delete(&event);

  if (!yaml_parser_parse(&parser, &event) ||
      (event.type != YAML_STREAM_END_EVENT)) {
    yaml_error(YAML_STREAM_END_EVENT, &event);
  }
  yaml_event_delete(&event);

  yaml_parser_delete(&parser);

  for (int i = 0; emph_classes[i]; i++)
    free(emph_classes[i]);
  free(emph_classes);
  lou_free();

  assert(!fclose(file));

  printf("%s (%d tests, %d failure%s)\n", (errors ? "FAILURE" : "SUCCESS"),
	 count, errors, ((errors != 1) ? "s" : ""));

  return errors ? 1 : 0;

#endif // not HAVE_LIBYAML
}

