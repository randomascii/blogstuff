import sys

# Don't change this because make.bat needs to know how many files to compile and link.
# Also, 16 works well for parallel compilation on my 4-core/8-thread laptop.
num_files = 16

# Number of classes per file.
num_classes = 50

# Number of virtual member functions per class.
num_members = 100

if len(sys.argv) > 1:
  num_members = int(sys.argv[1])

# Function size padding (characters to add to printf string).
num_chars = 1

for file_num in range(num_files):
  out = open('gen%d.cpp' % file_num, 'wt');

  out.write('#include <stdio.h>\n')
  out.write('\n')

  for i in range(num_classes):
    # Create a class number which is globally unique.
    class_num = file_num * num_classes + i;
    out.write('class Foo%d {\n' % class_num)
    out.write('public:\n')
    for member_num in range(num_members):
      out.write('  virtual void DoSomething%d() { printf("%d%s");}\n' % (member_num, class_num * num_members + member_num, '-' * num_chars))
    out.write('};\n')
    out.write('Foo%d* pFoo%d = new Foo%d();\n' % (class_num, class_num, class_num))
    out.write('\n')
  
  if file_num == 0:
    out.write('int main() {\n')
    out.write('}\n')
