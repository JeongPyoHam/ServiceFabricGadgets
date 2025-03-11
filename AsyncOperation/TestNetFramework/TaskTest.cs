using Microsoft.VisualStudio.TestTools.UnitTesting;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Security.Cryptography;
using System.Threading;
using System.Threading.Tasks;

namespace TestNetFramework
{
    public class TestTools
    {
        public static void WriteThread(string prefix) {
            var currThread = Thread.CurrentThread;
            Debug.WriteLine($"{prefix}: ThreadId:{currThread.ManagedThreadId} IsThreadPool:{currThread.IsThreadPoolThread}");
        }
    }

    class TestTaskFactory
    {
        static Random random = new Random();

        public static Task<int> Sum(int from, int to) {

            TestTools.WriteThread($"Sum: before Task.Run, [{from},{to}]");

            var task = Task.Run(async () => {

                TestTools.WriteThread($"Sum: inside before Delay, [{from},{to}]");

                await Task.Delay(random.Next() % 10000);
                
                TestTools.WriteThread($"Sum: inside Task.Run, [{from},{to}]");

                int sum = 0;
                for (int i = from; i <= to; ++i) {
                    sum += i;
                }
                return sum;
            });

            TestTools.WriteThread($"Sum: after Task.Run, [{from},{to}]");

            return task;
        }

        public static async Task DelayWait(Task task)
        {
            TestTools.WriteThread($"DelayWait: before Delay");

            await Task.Delay(random.Next() % 10000);

            TestTools.WriteThread($"DelayWait: after Delay");

            await task;

            TestTools.WriteThread($"DelayWait: after await");
        }

        public static Task Throw()
        {
            return Task.Run(async () =>
            {
                await Task.Delay(random.Next() % 10000);

                throw new Exception("I am designed to throw");
            });
        }
    }

    [TestClass]
    public class TaskTest
    {
        [TestMethod]
        public async Task WaitOne()
        {

            TestTools.WriteThread("Before await");

            var adder = TestTaskFactory.Sum(1, 1000);

            int sum = await adder;

            TestTools.WriteThread("After await");

            Assert.AreEqual(500500, sum);
        }

        [TestMethod]
        public async Task Nested_WaitOne()
        {
            TestTools.WriteThread("Nested: before await");

            var task = Task.Run(() => { return WaitOne(); });

            await task;

            TestTools.WriteThread("Nested: after await");
        }

        [TestMethod]
        public async Task Iteration()
        {
            int sum = 0;

            for (int i = 0; i < 10; i++)
            {
                int from = i * 100 + 1;
                int to = from + 99;

                TestTools.WriteThread("Iteration: before await");

                sum += await TestTaskFactory.Sum(from, to);

                TestTools.WriteThread("Iteration: before await");
            }

            Assert.AreEqual(500500, sum);
        }

        [TestMethod]
        public async Task Parallel()
        {
            List<Task<int>> tasks = new List<Task<int>>();

            for (int i = 0; i < 10; i++)
            {
                int from = i * 100 + 1;
                int to = from + 99;

                TestTools.WriteThread("Parallel: before Task List.Add");

                tasks.Add(TestTaskFactory.Sum(from, to));

                TestTools.WriteThread("Parallel: after Task List.Add");
            }

            var results = await Task.WhenAll(tasks);

            int sum = 0;
            foreach (int r in results)
            {
                sum += r;
            }

            Assert.AreEqual(500500, sum);
        }


        [TestMethod]
        public async Task WaitTheSame()
        {
            Task t = Task.Delay(10000);

            List<Task> tasks = new List<Task>();
            for (int i = 0; i <= 5; ++i)
            {
                tasks.Add(Task.Run(async () =>
                {
                    TestTools.WriteThread($"{nameof(WaitTheSame)}: {i} before await");

                    await t;

                    TestTools.WriteThread($"{nameof(WaitTheSame)}: {i} after await");
                }));
            }

            await Task.WhenAll(tasks);
        }
    }
}
