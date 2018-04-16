import errno
import os
import time

num_files = 1000

def FileStress(directory):
  try:
    os.makedirs(directory)
  except OSError as e:
    if e.errno != errno.EEXIST:
      raise
  os.chdir(directory)
  create_start_time = time.clock();
  for i in range(num_files):
    open('testfile%06d.txt' % i, 'wt').write('Hello world!')
  create_elapsed_time = time.clock() - create_start_time
  print '%.3f seconds to create %d files in %s.' % (create_elapsed_time, num_files, directory)

  delete_start_time = time.clock();
  for i in range(num_files):
    os.remove('testfile%06d.txt' % i)
  delete_elapsed_time = time.clock() - delete_start_time
  print '%.3f seconds to delete %d files in %s.' % (delete_elapsed_time, num_files, directory)
  return create_elapsed_time + delete_elapsed_time

start_dir = os.getcwd()

tracked_time = FileStress(os.path.join(start_dir, r'Trackedfiles\TestFiles'))

# Sleep for one second, to make it easier to identify the two tests in ETW traces.
time.sleep(0.5)

untracked_time = FileStress(os.path.join(start_dir, r'Untrackedfiles\TestFiles'))

print 'Tracked files take %1.2f times as long.' % (tracked_time / untracked_time)
