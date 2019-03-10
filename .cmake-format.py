# How wide to allow formatted cmake files
line_width = 120

# How many spaces to tab for indent
tab_size = 4

# If arglists are longer than this, break them always
max_subargs_per_line = 3

# If true, separate flow control names from their parentheses with a space
separate_ctrl_name_with_space = True

# If true, separate function names from parentheses with a space
separate_fn_name_with_space = True

# If a statement is wrapped to more than one line, than dangle the closing
# parenthesis on it's own line
dangle_parens = False

# What character to use for bulleted lists
bullet_char = '*'

# What character to use as punctuation after numerals in an enumerated list
enum_char = '.'

# What style line endings to use in the output.
line_ending = 'unix'

# Format command names consistently as 'lower' or 'upper' case
command_case = 'upper'

# Format keywords consistently as 'lower' or 'upper' case
keyword_case = 'upper'

# Specify structure for custom cmake functions
additional_commands = {
  "foo": {
    "flags": [
      "BAR",
      "BAZ"
    ],
    "kwargs": {
      "HEADERS": "*",
      "SOURCES": "*",
      "DEPENDS": "*"
    }
  }
}

# A list of command names which should always be wrapped
always_wrap = []

# Specify the order of wrapping algorithms during successive reflow attempts
algorithm_order = [0, 1, 2, 3]

# enable comment markup parsing and reflow
enable_markup = True

# If comment markup is enabled, don't reflow the first comment block in
# eachlistfile. Use this to preserve formatting of your
# copyright/licensestatements.
first_comment_is_literal = False

# If comment markup is enabled, don't reflow any comment block which matchesthis
# (regex) pattern. Default is `None` (disabled).
literal_comment_pattern = None

# Regular expression to match preformat fences in comments
# default=r'^\s*([`~]{3}[`~]*)(.*)$'
fence_pattern = '^\\s*([`~]{3}[`~]*)(.*)$'

# Regular expression to match rulers in comments
# default=r'^\s*[^\w\s]{3}.*[^\w\s]{3}$'
ruler_pattern = '^\\s*[^\\w\\s]{3}.*[^\\w\\s]{3}$'

# If true, emit the unicode byte-order mark (BOM) at the start of the file
emit_byteorder_mark = False

# A dictionary containing any per-command configuration overrides. Currently
# only `command_case` is supported.
per_command = {}

