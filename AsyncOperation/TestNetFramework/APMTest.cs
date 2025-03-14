using Microsoft.VisualStudio.TestTools.UnitTesting;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace TestNetFramework
{
    [TestClass]
    public class APMTest
    {
        byte[] buffer;

        [TestMethod]
        public void Sample()
        {
            string path = @"c:\temp\testfile.txt";

            using (var stream = new FileStream(path, FileMode.Create, FileAccess.ReadWrite))
            {
                var bytes = new UTF8Encoding(true).GetBytes("write this text in file");
                stream.Write(bytes, 0, bytes.Length);
            }

            AsyncCallback callback = delegate (IAsyncResult r) {

                TestTools.WriteThread("BeginRead Callback");

                string readFromFile = new UTF8Encoding(true).GetString(buffer);
                Debug.WriteLine(readFromFile);

                var stream = (FileStream) r.AsyncState;
                stream.Close();
                stream.Dispose();
            };

            var fs = new FileStream(path, FileMode.Open, FileAccess.Read);

            buffer = new byte[1024];

            TestTools.WriteThread("Before BeginRead");

            IAsyncResult asynOperation = fs.BeginRead(buffer, 0, buffer.Length, callback, fs);

            TestTools.WriteThread("After BeginRead");

            asynOperation.AsyncWaitHandle.WaitOne();
        }
    }
}
