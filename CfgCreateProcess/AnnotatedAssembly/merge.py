# Merge ETW samples into a disassembly. This assumes that your CPU Sampled data
# in WPA has just the address and sample-count columns. It assumes 64-bit
# addresses. It assumes a lot.

lines = open("samples.txt").readlines()[1:]
samples = {}
for line in lines:
  line, address, count = line.split(', ')
  # Convert from hexadecimal to an integer.
  address = int(address[2:], 16)
  samples[address] = int(count)

for line in open("disassembly_raw.txt").readlines():
  if line.startswith('ffff'):
    address = int(line[:17].replace('`', ''), 16)
    prefix = '      '
    if samples.has_key(address):
      prefix = '%5d ' % samples[address]
      del samples[address]
    print '%s%s' % (prefix, line.strip())
  else:
    print line.strip()

if len(samples) > 0:
  print '%d samples not matched up' % len(samples)
