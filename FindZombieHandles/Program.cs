/*
Copyright 2018 Google Inc. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

This code is heavily based on code by https://twitter.com/tiraniddo, author of NtApiDotNet
NtApiDotNet is available through NuGet - https://www.nuget.org/packages/NtApiDotNet/, and
through GitHub - https://github.com/google/sandbox-attacksurface-analysis-tools
*/

/*
This tool prints a list of processes that are holding handles to dead processes.
Briefly holding a handle to a dead process is necessary if you want to wait for
the process to die, or examine its return code, but holding on to a process
handle for more than a fraction of a second is unusual, for more than a few
seconds is almost always a bug. Some programs leak process handles and this
wastes an estimated 64 KB per handle, which adds up if you leak tens or hundreds
of thousands of handles.

Blog post is here:
https://randomascii.wordpress.com/2018/02/11/zombie-processes-are-eating-your-memory/
*/

using NtApiDotNet;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;

using HandleList = System.Collections.Generic.IEnumerable<NtApiDotNet.NtHandle>;

namespace FindZombieHandles
{
    class Program
    {
        private sealed class ZombieHandle : IDisposable
        {
            private readonly NtObject _object;

            public int Handle { get; }
            public string ProcessPath { get; }

            public ZombieHandle(NtObject obj, string process_path)
            {
                _object = obj;
                Handle = obj.Handle.DangerousGetHandle().ToInt32();
                ProcessPath = process_path;
            }

            public void Dispose()
            {
                ((IDisposable)_object).Dispose();
            }
        }

        static IEnumerable<ZombieHandle> GetThreads(NtProcess process)
        {
            string process_path = process.FullPath;
            int pid = process.ProcessId;

            List<ZombieHandle> objs = new List<ZombieHandle>
            {
                new ZombieHandle(process, process_path)
            };
            using (var query_process = NtProcess.Open(pid, ProcessAccessRights.QueryInformation, false))
            {
                if (query_process.IsSuccess)
                {
                    objs.AddRange(query_process.Result.GetThreads(ThreadAccessRights.QueryLimitedInformation)
                        .Select(t => new ZombieHandle(t, process_path)));
                }
            }
            return objs;
        }

        static bool FilterProcess(NtProcess process)
        {
            if (process.IsDeleting)
            {
                return true;
            }
            process.Close();
            return false;
        }

        static Dictionary<ulong, string> GetZombieProcessObjectAddress()
        {
            using (var ps = NtProcess.GetProcesses(ProcessAccessRights.QueryLimitedInformation).Where(FilterProcess).SelectMany(GetThreads).ToDisposableList())
            {
                var handles = ps.ToDictionary(p => p.Handle);
                var zombies = NtSystemInfo.GetHandles(NtProcess.Current.ProcessId, false).Where(h => handles.ContainsKey(h.Handle))
                    .ToDictionary(h => h.Object, h => handles[h.Handle].ProcessPath);
                Debug.Assert(handles.Count == zombies.Count);
                return zombies;
            }
        }

        static string GetProcessName(int process_id, bool verbose)
        {
            var image_path = NtSystemInfo.GetProcessIdImagePath(process_id, false);
            if (image_path.IsSuccess)
            {
                return verbose ? image_path.Result : Path.GetFileName(image_path.Result);
            }

            using (var process = NtProcess.Open(process_id, ProcessAccessRights.QueryLimitedInformation, false))
            {
                if (process.IsSuccess)
                {
                    if (verbose)
                        return process.Result.FullPath;
                    else
                        return process.Result.Name;
                }
                else
                {
                    try
                    {
                        return Process.GetProcessById(process_id).ProcessName;
                    }
                    catch (ArgumentException)
                    {
                    }
                }

                return "Unknown";
            }
        }

        static string ZombiePluralized(int count)
        {
            if (count == 1)
                return "zombie";
            else
                return "zombies";
        }

        static void Main(string[] args)
        {
            bool verbose = false;
            if (args.Length > 0 && args[0] == "-verbose")
                verbose = true;
            try
            {
                if (!NtToken.EnableDebugPrivilege())
                {
                    Console.WriteLine("WARNING: Can't enable debug privilege. Some zombies may not be found. Run as admin for full results.");
                }
                var zombies = GetZombieProcessObjectAddress();
                Console.WriteLine("{0} total zombie processes.", zombies.Count);
                if (zombies.Count == 0)
                    Console.WriteLine("No zombies found. Maybe all software is working correctly, but I doubt it. " +
                                      "More likely the zombie counting process failed for some reason. Please try again.");
                // Create a list to store the pids of process that hold zombies, a list of zombie handles,
                // a count of the zombies. The count is first for sorting.
                List<Tuple<int, int, HandleList>> count_and_pid = new List<Tuple<int, int, HandleList>>();
                foreach (var group in NtSystemInfo.GetHandles(-1, false).GroupBy(h => h.ProcessId))
                {
                    var total = group.Where(h => zombies.ContainsKey(h.Object));
                    int count = total.Count();

                    if (count > 0)
                    {
                        count_and_pid.Add(new Tuple<int, int, HandleList>(count, group.Key, total));
                    }
                }

                // Print the processes holding handles to zombies, sorted by zombie count.
                count_and_pid.Sort();
                count_and_pid.Reverse();
                foreach (Tuple<int, int, HandleList> buggy_process in count_and_pid)
                {
                    int count_by = buggy_process.Item1;
                    int pid = buggy_process.Item2;
                    HandleList total = buggy_process.Item3;
                    Console.WriteLine("    {0} {1} held by {2}({3})", count_by, ZombiePluralized(count_by), GetProcessName(pid, verbose), pid);
                    var names = total.GroupBy(h => zombies[h.Object], StringComparer.CurrentCultureIgnoreCase);
                    List<Tuple<int, string, int, int>> zombies_from_process = new List<Tuple<int, string, int, int>>();
                    foreach (var name in names)
                    {
                        int slash_index = name.Key.LastIndexOf('\\');
                        if (verbose)
                            slash_index = -1;
                        string process_name = name.Key.Substring(slash_index + 1);
                        int process_count = name.Count(h => h.ObjectType == "Process");
                        int thread_count = name.Count(h => h.ObjectType == "Thread");
                        zombies_from_process.Add(Tuple.Create(process_count + thread_count, process_name, process_count, thread_count));
                    }

                    // Print the processes being held as zombies, sorted by count.
                    zombies_from_process.Sort();
                    zombies_from_process.Reverse();
                    foreach (var zombie_process in zombies_from_process)
                    {
                        int total_count = zombie_process.Item1;
                        string process_name = zombie_process.Item2;
                        int process_count = zombie_process.Item3;
                        int thread_count = zombie_process.Item4;
                        
                        Console.WriteLine("        {0} {1} of {2} (process: {3} - thread: {4})", total_count, 
                            ZombiePluralized(total_count), process_name, process_count, thread_count);
                    }
                }
                if (!verbose)
                    Console.WriteLine("Pass -verbose to get full zombie names.");
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex.Message);
            }

            int[] pids = new int[3];
            if (GetConsoleProcessList(pids, 3) == 1)
            {
                Console.Write("Press any key to continue");
                Console.ReadKey();
            }
        }
        [System.Runtime.InteropServices.DllImport("kernel32.dll")]
        private static extern int GetConsoleProcessList(int[] buffer, int size);
    }
}
