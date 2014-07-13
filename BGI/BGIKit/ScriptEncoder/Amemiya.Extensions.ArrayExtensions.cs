using System;
using System.Collections.Generic;
using System.Linq;

namespace Amemiya.Extensions
{
    public static class ArrayExtensions
    {
        /// <summary>
        /// Combine two arrays into one
        /// </summary>
        /// <returns>The new array</returns>
        public static T[] CombineWith<T>(this T[] source, T[] with)
        {
            if (source == null || source.Length == 0)
                return with;

            var newBytes = new T[source.Length + with.Length];

            SetItems(newBytes, source, 0);
            SetItems(newBytes, with, source.Length);

            return newBytes;
        }

        /// <summary>
        /// Compare the two arrays
        /// </summary>
        /// <returns>True if they are equal</returns>
        public static bool EqualWith<T>(this T[] source, T[] compareWith)
        {
            if (source.Length != compareWith.Length)
                return false;

            if (source.Where((t, i) => !EqualityComparer<T>.Default.Equals(t, compareWith[i])).Any())
                return false;

            return true;
        }

        /// <summary>
        /// Expand current array to a lager one
        /// </summary>
        /// <returns>The new array</returns>
        public static byte[] ExpandTo(this byte[] source, int newLength)
        {
            if (source.Length > newLength)
                return source;

            var b = new byte[newLength];
            FillWith(b, (byte)0);

            SetItems(b, source, 0);

            return b;
        }

        /// <summary>
        /// Fill every item of current array with "with"
        /// </summary>
        public static void FillWith<T>(this T[] source, T with)
        {
            for (int i = 0; i < source.Length; i++)
                source[i] = with;
        }

        public static T[] CopyBlock<T>(this T[] bytesOrg, int intStart, int intLength)
        {
            try
            {
                var byteOutput = new T[intLength];

                for (int i = 0; i < intLength; i++)
                {
                    byteOutput[i] = bytesOrg[intStart + i];
                }
                return byteOutput;
            }
            catch (Exception e)
            {
                Console.WriteLine("{0}{1}", e.Message, intStart);
                return null;
            }
        }

        /// <summary>
        /// Locate an array index in current array
        /// </summary>
        /// <returns>-1 if not-found</returns>
        public static int IndexOf<T>(this T[] source, T[] arrayToFind, int startAt, bool isBackward)
        {
            if (isBackward)
            {
                for (int index = startAt - arrayToFind.Length; index >= 0; --index)
                {
                    if (EqualWith(CopyBlock(source, index, arrayToFind.Length), arrayToFind))
                        return index;
                }
                return -1;
            }

            for (int index = startAt; index <= (source.Length - arrayToFind.Length); ++index)
            {
                if (EqualWith(CopyBlock(source, index, arrayToFind.Length), arrayToFind))
                    return index;
            }
            return -1;
        }

        /// <summary>
        /// Set current array to bytesValue, starting from startFrom
        /// </summary>
        public static void SetItems<T>(this T[] source, T[] bytesValue, int startFrom)
        {
            if (source.Length < bytesValue.Length + startFrom)
                throw new Exception("dest too small");

            Array.Copy(bytesValue, 0, source, startFrom, bytesValue.Length);
        }

        public static T[] Slice<T>(this T[] source, int start, int end)
        {
            if (end < 0)
            {
                end = source.Length + end;
            }
            int len = end - start;

            T[] res = new T[len];
            for (int i = 0; i < len; i++)
            {
                res[i] = source[i + start];
            }
            return res;
        }
    }
}