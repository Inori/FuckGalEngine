using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace bExplorer
{
    public partial class 重命名 : Form
    {
        public static byte[] 编码文件名;
        public String 文件名 = "";
        public Encoding 编码 = Encoding.Default;

        public 重命名()
        {
            InitializeComponent();
        }

        public 重命名(String Name,Encoding Encode)
        {
            InitializeComponent();
            文件名 = Name;
            textBox1.Text = Name;
            textBox2.Text = Name;
            编码 = Encode;
            if (编码.CodePage == 932)
            {
                comboBox1.SelectedIndex = 0;
            }
            else if (编码.CodePage == 936)
            {
                comboBox1.SelectedIndex = 1;
            }
            
        }

        public 重命名(String Name)
        {
            InitializeComponent();
            文件名 = Name;
            textBox1.Text = Name;
            textBox2.Text = Name;
        }

        private void button1_Click(object sender, EventArgs e)
        {
            编码文件名 = 编码.GetBytes(textBox1.Text);
            this.Close();
        }

        private void comboBox1_SelectedIndexChanged(object sender, EventArgs e)
        {
            编码 = Encoding.GetEncoding(comboBox1.SelectedIndex == 0 ? 932 : 936);
        }

        private void button2_Click(object sender, EventArgs e)
        {
            this.Close();
        }

        private void 重命名_Load(object sender, EventArgs e)
        {
            comboBox1.SelectedIndex = 1;
        }


    }
}
