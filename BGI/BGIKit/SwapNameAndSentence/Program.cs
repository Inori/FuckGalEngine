using System;
using System.IO;
using System.Text;

namespace SwapNameAndSentence
{
    internal class Program
    {
        private static void Main(string[] args)
        {
            if (args.Length == 0)
            {
                Console.WriteLine("Usage: SwapNameAndSentence.exe <input>");
                return;
            }

            string strFileInput = args[0];

            var sr = new StreamReader(strFileInput, Encoding.UTF8, true);
            var sw = new StreamWriter(strFileInput + ".new", false);

            var lines = sr.ReadToEnd().Replace("\r\n", "\n").Split('\n');

            for (int i = 0; i < lines.Length; )
            {
                if (lines[i].Contains(">「") && lines[i].EndsWith("」"))
                {
                    sw.WriteLine(lines[i + 1]);
                    sw.WriteLine(lines[i]);
                    i += 2;
                }
                else
                {
                    sw.WriteLine(lines[i]);
                    i++;
                }
            }

            sw.Close();
            sr.Close();

            File.Move(strFileInput, strFileInput + ".old");
            File.Move(strFileInput + ".new", strFileInput);
        }
    }
}
