using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Windows.Forms;
using System.Runtime.InteropServices;
using System.Text.RegularExpressions;

namespace bExplorer
{


    public partial class B浏览器 : Form
    {
        public static Encoding 编码 = Encoding.GetEncoding(932);
        public Encoding jis = Encoding.GetEncoding(932);
        public List<bFile> b文件组 = new List<bFile>();
        public bFile 选择b文件;
        public String 打开的文件;
        public FileStream 打开b文件;
        List<int> X = new List<int>();
        List<int> Y = new List<int>();

        public B浏览器()
        {
            InitializeComponent();
        }

        public string 规则化文件名(string fileName)
        {
            if (string.IsNullOrEmpty(fileName))
                return fileName;
            string invalidChars = new string(Path.GetInvalidFileNameChars());
            string invalidReStr = string.Format("[{0}]", Regex.Escape(invalidChars));
            return Regex.Replace(fileName, invalidReStr, "");
        } 
        


        public bFile 读取b(Stream bFileStream)
        {
            bFile b文件 = new bFile();
            byte[] buff;
            bFileStream.Read(b文件.Hdr, 0, 16);



            //System.Diagnostics.Debug.WriteLine(jis.GetString(b文件.Hdr));
            if (jis.GetString(b文件.Hdr).Substring(0, 6) == "abmp11" || jis.GetString(b文件.Hdr).Substring(0, 6) == "abmp12")
            {
                bFileStream.Read(b文件.bData.Hdr, 0, 16);
                if (jis.GetString(b文件.bData.Hdr).Substring(0, 8) == "abdata10" ||
                    jis.GetString(b文件.bData.Hdr).Substring(0, 8) == "abdata11" ||
                    jis.GetString(b文件.bData.Hdr).Substring(0, 8) == "abdata12" ||
                    jis.GetString(b文件.bData.Hdr).Substring(0, 8) == "abdata13" ||
                    jis.GetString(b文件.bData.Hdr).Substring(0, 8) == "abdata14")
                {
                    buff = new byte[4];
                    bFileStream.Read(buff, 0, 4);
                    b文件.bData.offset = BitConverter.ToInt32(buff, 0);

                    b文件.bData.Data = new byte[b文件.bData.offset];
                    bFileStream.Read(b文件.bData.Data, 0, b文件.bData.offset);

                    while (bFileStream.Position < bFileStream.Length - 1)
                    {
                        bImage b图像 = new bImage();
                        bFileStream.Read(b图像.Hdr, 0, 16);
                        System.Diagnostics.Debug.WriteLine(编码.GetString(b图像.Hdr));
                        if (jis.GetString(b图像.Hdr).Substring(0, 9) == "abimage10")
                        {
                            b图像.nData = (byte)bFileStream.ReadByte();
                            for (int i = 0; i < b图像.nData; i++)
                            {
                                bImgData b数据 = new bImgData();
                                bFileStream.Read(b数据.Hdr, 0, 16);

                                //System.Diagnostics.Debug.Write(bFileStream.Position);

                                var header = jis.GetString(b数据.Hdr).Substring(0, 10);
                                if (header == "abimgdat15" ||
                                    header == "abimgdat14" ||
                                    header == "abimgdat13" ||
                                    header == "abimgdat11")
                                {
                                    if (header == "abimgdat15")
                                    {
                                        b数据.Unkown15 = new byte[4];
                                        bFileStream.Read(b数据.Unkown15, 0, b数据.Unkown15.Length);
                                    }

                                    buff = new byte[2];
                                    bFileStream.Read(buff, 0, 2);
                                    b数据.NameLen = BitConverter.ToInt16(buff, 0);
                                    if (header == "abimgdat15")
                                    {
                                        b数据.Name = new byte[b数据.NameLen * 2];
                                        bFileStream.Read(b数据.Name, 0, b数据.NameLen * 2);
                                    }
                                    else
                                    {
                                        b数据.Name = new byte[b数据.NameLen];
                                        bFileStream.Read(b数据.Name, 0, b数据.NameLen);
                                    }

                                    bFileStream.Read(buff, 0, 2);
                                    b数据.HashLen = BitConverter.ToInt16(buff, 0);

                                    if (b数据.HashLen != 0)
                                    {
                                        b数据.Hash = new byte[b数据.HashLen];
                                        bFileStream.Read(b数据.Hash, 0, b数据.HashLen);
                                    }

                                    if (header == "abimgdat14")
                                    {
                                        b数据.Unkown = new byte[77];
                                        bFileStream.Read(b数据.Unkown, 0, b数据.Unkown.Length);
                                    }
                                    else if (header == "abimgdat13")
                                    {
                                        b数据.Unkown = new byte[13];
                                        bFileStream.Read(b数据.Unkown, 0, b数据.Unkown.Length);
                                    }
                                    else if (header == "abimgdat15")
                                    {
                                        b数据.Unkown = new byte[18];
                                        bFileStream.Read(b数据.Unkown, 0, b数据.Unkown.Length);
                                    }
                                    else
                                    {
                                        b数据.Unkown = new byte[1];
                                        bFileStream.Read(b数据.Unkown, 0, b数据.Unkown.Length);
                                    }

                                    buff = new byte[4];
                                    bFileStream.Read(buff, 0, 4);
                                    b数据.DataLen = BitConverter.ToInt32(buff,0);
                                    

                                    System.Diagnostics.Debug.WriteLine(bFileStream.Position);
                                    buff = new byte[b数据.DataLen];
                                    bFileStream.Read(buff, 0, b数据.DataLen);
                                    b数据.Data = new MemoryStream(buff);
                                    System.Diagnostics.Debug.WriteLine(bFileStream.Position);

                                    b图像.DataList.Add(b数据);
                                }
                                else
                                {
                                    throw (new Exception("不是标准的b文件."));
                                }

                            }
                            b文件.ImageList.Add(b图像);
                        }
                        else if (编码.GetString(b图像.Hdr).Substring(0, 9) == "absound10")
                        {
                            b文件.bEnd = new byte[bFileStream.Length - bFileStream.Position + 0x10];
                            bFileStream.Seek(-0x10, SeekOrigin.Current);
                            bFileStream.Read(b文件.bEnd, 0, b文件.bEnd.Length);
                        }
                        else
                        {
                            throw (new Exception("不是标准的b文件."));
                        }

                    }
                    
                }
                else
                {
                    throw (new Exception("不是标准的b文件."));
                }
            }
            else
            {
                throw (new Exception("不是标准的b文件."));
            }
            
            return b文件;
        }

        public void 显示列表(ListView 列表,bFile b文件)
        {
            列表.Items.Clear();

            if (b文件.Parent != null)
            {
                ListViewItem li =new ListViewItem(".");
                li.SubItems.Add("...");
                列表.Items.Add(li);
            }
            int i = 0;
            int n = 0;
            foreach (bImage b图像 in b文件.ImageList)
            {
                foreach(bImgData b数据 in b图像.DataList)
                {
                    ListViewItem li = new ListViewItem(n.ToString() + "-" + i.ToString());
                    li.SubItems.Add(b数据.GetName());
                    li.SubItems.Add(b数据.GetFileType());
                    li.SubItems.Add(b数据.DataLen.ToString());
                    列表.Items.Add(li);
                    i++;
                }
                n++;
            }
        }

        private void B浏览器_Load(object sender, EventArgs e)
        {
            comboBox1.SelectedIndex = 0;
        }
        

        private void button1_Click(object sender, EventArgs e)
        {
            打开文件.FileName = "";
            打开文件.ShowDialog(this);
            if (打开文件.FileName != "")
            {
                if (打开b文件 != null)
                {
                    打开b文件.Close();
                }

                打开的文件 = 打开文件.FileName;
                
                b文件组.Clear();
                X.Clear();
                Y.Clear();
                打开b文件 = new FileStream(打开文件.FileName, FileMode.Open);
                b文件组.Add(读取b(打开b文件));
                选择b文件 = b文件组[0];
                显示列表(浏览器, 选择b文件);
            }
        }

        private void 浏览器_MouseDoubleClick(object sender, MouseEventArgs e)
        {
            ListViewHitTestInfo info = 浏览器.HitTest(e.X, e.Y);

            if (info.Item != null)
            {
                int x,y;
                if (info.Item.SubItems[0].Text == ".")
                {
                    byte[] buff = 获取b(选择b文件);
                    选择b文件.Parent.ImageList[X[X.Count - 1]].DataList[Y[Y.Count - 1]].Data = new MemoryStream(buff);
                    选择b文件.Parent.ImageList[X[X.Count - 1]].DataList[Y[Y.Count - 1]].DataLen = buff.Length;
                    X.RemoveAt(X.Count - 1);
                    Y.RemoveAt(Y.Count - 1);
                    b文件组.Remove(选择b文件);
                    选择b文件 = 选择b文件.Parent;
                    显示列表(浏览器, 选择b文件);
                }
                else
                {
                    string[] 位置 = info.Item.SubItems[0].Text.Split('-');
                    x = Convert.ToInt32(位置[0]);
                    y = Convert.ToInt32(位置[1]);
                    if (info.Item.SubItems[2].Text == "b")
                    {
                        string[] 总位置 = info.Item.SubItems[0].Text.Split('-');
                        X.Add(Convert.ToInt32(总位置[0]));
                        Y.Add(Convert.ToInt32(总位置[1]));
                        bFile b文件 = 读取b(选择b文件.ImageList[x].DataList[y].Data);
                        b文件.Parent = 选择b文件;
                        选择b文件 = b文件;
                        b文件组.Add(b文件);
                        显示列表(浏览器, 选择b文件);
                    }
                    else
                    {
                        string 临时文件 = System.IO.Path.GetTempPath() + 规则化文件名(选择b文件.ImageList[x].DataList[y].GetName()) + "." + 选择b文件.ImageList[x].DataList[y].GetFileType();

                        选择b文件.ImageList[x].DataList[y].Data.Seek(0, SeekOrigin.Begin);
                        byte[] buff = new byte[选择b文件.ImageList[x].DataList[y].Data.Length];
                        选择b文件.ImageList[x].DataList[y].Data.Read(buff, 0, (int)选择b文件.ImageList[x].DataList[y].Data.Length);
                        选择b文件.ImageList[x].DataList[y].Data.Seek(0, SeekOrigin.Begin);
                        File.WriteAllBytes(临时文件, buff);
                        System.Diagnostics.ProcessStartInfo Info = new System.Diagnostics.ProcessStartInfo(临时文件);
                        System.Diagnostics.Process Pro = System.Diagnostics.Process.Start(Info);
                    }
                }
                

                
            }
        }

        private void 提取_Click(object sender, EventArgs e)
        {
            string[] 位置 = 浏览器.SelectedItems[0].SubItems[0].Text.Split('-');
            int x = Convert.ToInt32(位置[0]);
            int y = Convert.ToInt32(位置[1]);
            保存文件.DefaultExt = 选择b文件.ImageList[x].DataList[y].GetFileType();
            保存文件.FileName = "";
            保存文件.ShowDialog(this);

            if (保存文件.FileName != "")
            {
                选择b文件.ImageList[x].DataList[y].Data.Seek(0, SeekOrigin.Begin);
                byte[] buff = new byte[选择b文件.ImageList[x].DataList[y].Data.Length];
                选择b文件.ImageList[x].DataList[y].Data.Read(buff, 0, (int)选择b文件.ImageList[x].DataList[y].Data.Length);
                选择b文件.ImageList[x].DataList[y].Data.Seek(0, SeekOrigin.Begin);
                File.WriteAllBytes(保存文件.FileName, buff);
                MessageBox.Show("提取完成!", "成功", MessageBoxButtons.OK, MessageBoxIcon.Asterisk);
            }
        }

        private void 替换_Click(object sender, EventArgs e)
        {
            string[] 位置 = 浏览器.SelectedItems[0].SubItems[0].Text.Split('-');
            int x = Convert.ToInt32(位置[0]);
            int y = Convert.ToInt32(位置[1]);
            打开文件.Filter = "";
            打开文件.FileName = "";
            打开文件.ShowDialog(this);

            if (打开文件.FileName != "")
            {
                byte[] buff = File.ReadAllBytes(打开文件.FileName);
                选择b文件.ImageList[x].DataList[y].DataLen = buff.Length;
                选择b文件.ImageList[x].DataList[y].Data = new MemoryStream(buff);
                显示列表(浏览器, 选择b文件);
            }
        }

        private void 保存_Click(object sender, EventArgs e)
        {
            int i = 1;
            bFile b文件 = 选择b文件;
            while (选择b文件.Parent != null)
            {
                byte[] buff = 获取b(选择b文件);
                选择b文件.Parent.ImageList[X[X.Count - i]].DataList[Y[Y.Count - i]].Data = new MemoryStream(buff);
                选择b文件.Parent.ImageList[X[X.Count - i]].DataList[Y[Y.Count - i]].DataLen = buff.Length;
                i++;
                选择b文件 = 选择b文件.Parent;
            }
            选择b文件 = b文件;
            保存b(打开b文件);

            b文件组.Clear();
            X.Clear();
            Y.Clear();
            b文件组.Add(读取b(打开b文件));
            选择b文件 = b文件组[0];
            显示列表(浏览器, 选择b文件);

            MessageBox.Show("保存完成!", "成功", MessageBoxButtons.OK, MessageBoxIcon.Asterisk);
        }

        public byte[] 获取b (bFile b文件)
        {
            FileStream 文件流 = new FileStream(System.IO.Path.GetTempPath() + "temp.b", FileMode.Create);
            文件流.Write(b文件.Hdr, 0, b文件.Hdr.Length);
            文件流.Write(b文件.bData.Hdr, 0, b文件.bData.Hdr.Length);
            文件流.Write(BitConverter.GetBytes(b文件.bData.offset), 0, BitConverter.GetBytes(b文件.bData.offset).Length);
            文件流.Write(b文件.bData.Data, 0, b文件.bData.Data.Length);
            foreach (bImage b图像 in b文件.ImageList)
            {
                文件流.Write(b图像.Hdr, 0, b图像.Hdr.Length);
                文件流.WriteByte(b图像.nData);
                foreach (bImgData b数据 in b图像.DataList)
                {
                    文件流.Write(b数据.Hdr, 0, b图像.Hdr.Length);
                    if (jis.GetString(b数据.Hdr).Substring(0, 10) == "abimgdat15")
                    {
                        文件流.Write(b数据.Unkown15, 0, b数据.Unkown15.Length);
                    }
                    文件流.Write(BitConverter.GetBytes(b数据.NameLen), 0, BitConverter.GetBytes(b数据.NameLen).Length);
                    文件流.Write(b数据.Name, 0, b数据.Name.Length);
                    文件流.Write(BitConverter.GetBytes(b数据.HashLen), 0, BitConverter.GetBytes(b数据.HashLen).Length);
                    if (b数据.HashLen != 0)
                    {
                        文件流.Write(b数据.Hash, 0, b数据.Hash.Length);
                    }
                    文件流.Write(b数据.Unkown, 0, b数据.Unkown.Length);
                    文件流.Write(BitConverter.GetBytes(b数据.DataLen), 0, BitConverter.GetBytes(b数据.DataLen).Length);
                    b数据.Data.Seek(0, SeekOrigin.Begin);
                    byte[] buff = new byte[b数据.Data.Length];
                    b数据.Data.Read(buff, 0, (int)b数据.Data.Length);
                    b数据.Data.Seek(0, SeekOrigin.Begin);
                    文件流.Write(buff, 0, buff.Length);
                }
            }
            文件流.Write(b文件.bEnd, 0, b文件.bEnd.Length);

            byte[] buff2 = new byte[文件流.Length];
            文件流.Seek(0, SeekOrigin.Begin);
            文件流.Read(buff2, 0, (int)文件流.Length);
            文件流.Seek(0, SeekOrigin.Begin);
            文件流.Close();
            return buff2;
        }

        public void 保存b(string 路径)
        {
            FileStream 文件流 = new FileStream(路径, FileMode.Create);
            文件流.Write(b文件组[0].Hdr, 0, b文件组[0].Hdr.Length);
            文件流.Write(b文件组[0].bData.Hdr, 0, b文件组[0].bData.Hdr.Length);
            文件流.Write(BitConverter.GetBytes(b文件组[0].bData.offset), 0, BitConverter.GetBytes(b文件组[0].bData.offset).Length);
            文件流.Write(b文件组[0].bData.Data, 0, b文件组[0].bData.Data.Length);
            foreach (bImage b图像 in b文件组[0].ImageList)
            {
                文件流.Write(b图像.Hdr, 0, b图像.Hdr.Length);
                文件流.WriteByte(b图像.nData);
                foreach (bImgData b数据 in b图像.DataList)
                {
                    文件流.Write(b数据.Hdr, 0, b图像.Hdr.Length);
                    if (jis.GetString(b数据.Hdr).Substring(0, 10) == "abimgdat15")
                    {
                        文件流.Write(b数据.Unkown15, 0, b数据.Unkown15.Length);
                    }
                    文件流.Write(BitConverter.GetBytes(b数据.NameLen), 0, BitConverter.GetBytes(b数据.NameLen).Length);
                    文件流.Write(b数据.Name, 0, b数据.Name.Length);
                    文件流.Write(BitConverter.GetBytes(b数据.HashLen), 0, BitConverter.GetBytes(b数据.HashLen).Length);
                    if (b数据.HashLen != 0)
                    {
                        文件流.Write(b数据.Hash, 0, b数据.Hash.Length);
                    }
                    文件流.Write(b数据.Unkown, 0, b数据.Unkown.Length);
                    文件流.Write(BitConverter.GetBytes(b数据.DataLen), 0, BitConverter.GetBytes(b数据.DataLen).Length);
                    b数据.Data.Seek(0, SeekOrigin.Begin);
                    byte[] buff = new byte[b数据.Data.Length];
                    b数据.Data.Read(buff, 0, (int)b数据.Data.Length);
                    b数据.Data.Seek(0, SeekOrigin.Begin);
                    文件流.Write(buff, 0, buff.Length);
                }
            }
            文件流.Write(b文件组[0].bEnd, 0, b文件组[0].bEnd.Length);
            文件流.Close();
        }


        public void 保存b(FileStream 文件流)
        {
            文件流.Seek(0, SeekOrigin.Begin);
            文件流.Write(b文件组[0].Hdr, 0, b文件组[0].Hdr.Length);
            文件流.Write(b文件组[0].bData.Hdr, 0, b文件组[0].bData.Hdr.Length);
            文件流.Write(BitConverter.GetBytes(b文件组[0].bData.offset), 0, BitConverter.GetBytes(b文件组[0].bData.offset).Length);
            文件流.Write(b文件组[0].bData.Data, 0, b文件组[0].bData.Data.Length);
            foreach (bImage b图像 in b文件组[0].ImageList)
            {
                文件流.Write(b图像.Hdr, 0, b图像.Hdr.Length);
                文件流.WriteByte(b图像.nData);
                foreach (bImgData b数据 in b图像.DataList)
                {
                    文件流.Write(b数据.Hdr, 0, b图像.Hdr.Length);
                    if (jis.GetString(b数据.Hdr).Substring(0, 10) == "abimgdat15")
                    {
                        文件流.Write(b数据.Unkown15, 0, b数据.Unkown15.Length);
                    }
                    文件流.Write(BitConverter.GetBytes(b数据.NameLen), 0, BitConverter.GetBytes(b数据.NameLen).Length);
                    文件流.Write(b数据.Name, 0, b数据.Name.Length);
                    文件流.Write(BitConverter.GetBytes(b数据.HashLen), 0, BitConverter.GetBytes(b数据.HashLen).Length);
                    if (b数据.HashLen != 0)
                    {
                        文件流.Write(b数据.Hash, 0, b数据.Hash.Length);
                    }
                    文件流.Write(b数据.Unkown, 0, b数据.Unkown.Length);
                    文件流.Write(BitConverter.GetBytes(b数据.DataLen), 0, BitConverter.GetBytes(b数据.DataLen).Length);
                    b数据.Data.Seek(0, SeekOrigin.Begin);
                    byte[] buff = new byte[b数据.Data.Length];
                    b数据.Data.Read(buff, 0, (int)b数据.Data.Length);
                    b数据.Data.Seek(0, SeekOrigin.Begin);
                    文件流.Write(buff, 0, buff.Length);
                }
            }
            文件流.Write(b文件组[0].bEnd, 0, b文件组[0].bEnd.Length);
            文件流.Seek(0, SeekOrigin.Begin);
        }

        private void 另存为_Click(object sender, EventArgs e)
        {

            int i = 1;
            bFile b文件 = 选择b文件;
            while (选择b文件.Parent != null)
            {
                byte[] buff = 获取b(选择b文件);
                选择b文件.Parent.ImageList[X[X.Count - i]].DataList[Y[Y.Count - i]].Data = new MemoryStream(buff);
                选择b文件.Parent.ImageList[X[X.Count - i]].DataList[Y[Y.Count - i]].DataLen = buff.Length;
                i++;
                选择b文件 = 选择b文件.Parent;
            }
            选择b文件 = b文件;

            保存文件.DefaultExt = "b";
            保存文件.FileName = "";
            保存文件.ShowDialog(this);
            if (保存文件.FileName != "")
            {
                保存b(保存文件.FileName);
                MessageBox.Show("保存完成!", "成功", MessageBoxButtons.OK, MessageBoxIcon.Asterisk);
            }
        }

        private void button2_Click(object sender, EventArgs e)
        {
            MessageBox.Show(
                "bExplorer Ver.0.10\r\n"+
                "bExplorer 可以对Qlie引擎的b文件(*.b)进行编辑,可以提取b文件中的图片,支持双击查看文件,支持多层b文件浏览.\r\n"+
                "bExplorer 用于Qlie引擎游戏 \"空飛ぶ羊と真夏の花\" 与其他Qlie引擎游戏(?).\r\n\r\n"+

                "By: HIGAN\r\n" +
                "E-mail: higan@live.cn\r\n" +
                "Website: blog.higan.me\r\n",
                "关于bExplorer",MessageBoxButtons.OK,MessageBoxIcon.Asterisk);
        }

        private void 拖放文件(object sender, DragEventArgs e)
        {
            Array aryFiles = ((System.Array)e.Data.GetData(DataFormats.FileDrop));
            String File = aryFiles.GetValue(0).ToString();
            if (打开b文件 != null)
            {
                打开b文件.Close();
            }

            打开的文件 = File;

            b文件组.Clear();
            X.Clear();
            Y.Clear();
            打开b文件 = new FileStream(File, FileMode.Open);
            b文件组.Add(读取b(打开b文件));
            选择b文件 = b文件组[0];
            显示列表(浏览器, 选择b文件);
        }

        private void 拖放开始(object sender, DragEventArgs e)
        {
            if (e.Data.GetDataPresent(DataFormats.FileDrop))
                e.Effect = DragDropEffects.Link;
            else e.Effect = DragDropEffects.None;
        }

        private void comboBox1_SelectedIndexChanged(object sender, EventArgs e)
        {
            编码 = Encoding.GetEncoding(comboBox1.SelectedIndex == 0 ? 932 : 936);
            if(选择b文件 != null)
            {
                显示列表(浏览器, 选择b文件);
            }
        }

        private void button3_Click(object sender, EventArgs e)
        {
            string[] 位置 = 浏览器.SelectedItems[0].SubItems[0].Text.Split('-');
            int x = Convert.ToInt32(位置[0]);
            int y = Convert.ToInt32(位置[1]);
            (new 重命名(选择b文件.ImageList[x].DataList[y].GetName())).ShowDialog(this);
            
            选择b文件.ImageList[x].DataList[y].Name = new byte[重命名.编码文件名.Length];
            重命名.编码文件名.CopyTo(选择b文件.ImageList[x].DataList[y].Name, 0);
            选择b文件.ImageList[x].DataList[y].NameLen = (short)重命名.编码文件名.Length;
            显示列表(浏览器, 选择b文件);
        }

        private void 浏览器_SelectedIndexChanged(object sender, EventArgs e)
        {

        }
    }

    public class bFile
    {
        public bFile Parent;
        public byte[] Hdr = new byte[16];
        public bData bData = new bData();
        public List<bImage> ImageList = new List<bImage>();
        public byte[] bEnd;

        public bFile()
        {
            Parent = null;
        }

        public bFile(bFile 父)
        {
            Parent = 父;
        }
    }

    public class bData
    {
        public byte[] Hdr = new byte[16];
        public int offset;
        public byte[] Data;

        public String Header => Encoding.ASCII.GetString(Hdr);
    }

    public class bImage
    {
        public byte[] Hdr = new byte[16];
        public byte nData;
        public List<bImgData> DataList = new List<bImgData>();

        public String Header => Encoding.ASCII.GetString(Hdr);
    }

    public class bImgData
    {
        public byte[] Hdr = new byte[16];
        public short NameLen;
        public byte[] Name;
        public short HashLen;
        public byte[] Hash;
        public byte[] Unkown;
        public byte[] Unkown15;
        public int DataLen;
        public MemoryStream Data;
        public String Header => Encoding.ASCII.GetString(Hdr);

        public string GetName()
        {
            var encoding = B浏览器.编码;

            if (Encoding.GetEncoding(932).GetString(Hdr).Substring(0, 10) == "abimgdat15")
            {
                encoding = Encoding.Unicode;
            }

            return encoding.GetString(Name);
        }


        public string GetFileType()
        {
            byte [] buff = new byte[2];
            Data.Seek(0, SeekOrigin.Begin);
            Data.Read(buff,0,2);
            Data.Seek(0, SeekOrigin.Begin);
            string ext;
            string hdr = buff[0].ToString("X") + buff[1].ToString("X");


            if (hdr == "8950")
            {
                ext = "png";
            }
            else if (hdr == "424D")
            {
                ext = "bmp";
            }
            else if (hdr == "FFD8")
            {
                ext = "jpg";
            }
            else if (hdr == "494D")
            {
                ext = "imoavi";
            }
            else if (hdr == "6162")
            {
                ext = "b";
            }
            else if (hdr == "4F67")
            {
                ext = "ogg";
            }
            else
            {
                ext = "UnKown";
            }
            Data.Seek(0,SeekOrigin.Begin);
            return ext;

        }
    }
}
