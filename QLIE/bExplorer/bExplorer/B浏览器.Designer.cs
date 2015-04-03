namespace bExplorer
{
    partial class B浏览器
    {
        /// <summary>
        /// 必需的设计器变量。
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// 清理所有正在使用的资源。
        /// </summary>
        /// <param name="disposing">如果应释放托管资源，为 true；否则为 false。</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows 窗体设计器生成的代码

        /// <summary>
        /// 设计器支持所需的方法 - 不要
        /// 使用代码编辑器修改此方法的内容。
        /// </summary>
        private void InitializeComponent()
        {
            this.浏览器 = new System.Windows.Forms.ListView();
            this.序号 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.文件名 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.类型 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.大小 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.button1 = new System.Windows.Forms.Button();
            this.打开文件 = new System.Windows.Forms.OpenFileDialog();
            this.保存文件 = new System.Windows.Forms.SaveFileDialog();
            this.保存 = new System.Windows.Forms.Button();
            this.提取 = new System.Windows.Forms.Button();
            this.替换 = new System.Windows.Forms.Button();
            this.另存为 = new System.Windows.Forms.Button();
            this.button2 = new System.Windows.Forms.Button();
            this.button3 = new System.Windows.Forms.Button();
            this.comboBox1 = new System.Windows.Forms.ComboBox();
            this.SuspendLayout();
            // 
            // 浏览器
            // 
            this.浏览器.AllowDrop = true;
            this.浏览器.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.序号,
            this.文件名,
            this.类型,
            this.大小});
            this.浏览器.FullRowSelect = true;
            this.浏览器.Location = new System.Drawing.Point(12, 12);
            this.浏览器.Name = "浏览器";
            this.浏览器.Size = new System.Drawing.Size(518, 363);
            this.浏览器.TabIndex = 0;
            this.浏览器.UseCompatibleStateImageBehavior = false;
            this.浏览器.View = System.Windows.Forms.View.Details;
            this.浏览器.DragDrop += new System.Windows.Forms.DragEventHandler(this.拖放文件);
            this.浏览器.DragEnter += new System.Windows.Forms.DragEventHandler(this.拖放开始);
            this.浏览器.MouseDoubleClick += new System.Windows.Forms.MouseEventHandler(this.浏览器_MouseDoubleClick);
            // 
            // 序号
            // 
            this.序号.Text = "序号";
            this.序号.Width = 44;
            // 
            // 文件名
            // 
            this.文件名.Text = "文件名";
            this.文件名.Width = 280;
            // 
            // 类型
            // 
            this.类型.Text = "类型";
            this.类型.Width = 130;
            // 
            // 大小
            // 
            this.大小.Text = "大小";
            // 
            // button1
            // 
            this.button1.Location = new System.Drawing.Point(536, 12);
            this.button1.Name = "button1";
            this.button1.Size = new System.Drawing.Size(75, 23);
            this.button1.TabIndex = 1;
            this.button1.Text = "打开";
            this.button1.UseVisualStyleBackColor = true;
            this.button1.Click += new System.EventHandler(this.button1_Click);
            // 
            // 打开文件
            // 
            this.打开文件.Filter = "B文件|*.b";
            // 
            // 保存
            // 
            this.保存.Location = new System.Drawing.Point(536, 41);
            this.保存.Name = "保存";
            this.保存.Size = new System.Drawing.Size(75, 23);
            this.保存.TabIndex = 2;
            this.保存.Text = "保存";
            this.保存.UseVisualStyleBackColor = true;
            this.保存.Click += new System.EventHandler(this.保存_Click);
            // 
            // 提取
            // 
            this.提取.Location = new System.Drawing.Point(536, 155);
            this.提取.Name = "提取";
            this.提取.Size = new System.Drawing.Size(75, 23);
            this.提取.TabIndex = 3;
            this.提取.Text = "提取";
            this.提取.UseVisualStyleBackColor = true;
            this.提取.Click += new System.EventHandler(this.提取_Click);
            // 
            // 替换
            // 
            this.替换.Location = new System.Drawing.Point(536, 184);
            this.替换.Name = "替换";
            this.替换.Size = new System.Drawing.Size(75, 23);
            this.替换.TabIndex = 4;
            this.替换.Text = "替换";
            this.替换.UseVisualStyleBackColor = true;
            this.替换.Click += new System.EventHandler(this.替换_Click);
            // 
            // 另存为
            // 
            this.另存为.Location = new System.Drawing.Point(537, 70);
            this.另存为.Name = "另存为";
            this.另存为.Size = new System.Drawing.Size(75, 23);
            this.另存为.TabIndex = 5;
            this.另存为.Text = "另存为";
            this.另存为.UseVisualStyleBackColor = true;
            this.另存为.Click += new System.EventHandler(this.另存为_Click);
            // 
            // button2
            // 
            this.button2.Location = new System.Drawing.Point(537, 276);
            this.button2.Name = "button2";
            this.button2.Size = new System.Drawing.Size(75, 23);
            this.button2.TabIndex = 6;
            this.button2.Text = "关于";
            this.button2.UseVisualStyleBackColor = true;
            this.button2.Click += new System.EventHandler(this.button2_Click);
            // 
            // button3
            // 
            this.button3.Location = new System.Drawing.Point(536, 126);
            this.button3.Name = "button3";
            this.button3.Size = new System.Drawing.Size(75, 23);
            this.button3.TabIndex = 7;
            this.button3.Text = "重命名";
            this.button3.UseVisualStyleBackColor = true;
            this.button3.Click += new System.EventHandler(this.button3_Click);
            // 
            // comboBox1
            // 
            this.comboBox1.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.comboBox1.FormattingEnabled = true;
            this.comboBox1.Items.AddRange(new object[] {
            "日语",
            "中文"});
            this.comboBox1.Location = new System.Drawing.Point(537, 250);
            this.comboBox1.Name = "comboBox1";
            this.comboBox1.Size = new System.Drawing.Size(74, 20);
            this.comboBox1.TabIndex = 8;
            this.comboBox1.SelectedIndexChanged += new System.EventHandler(this.comboBox1_SelectedIndexChanged);
            // 
            // B浏览器
            // 
            this.AllowDrop = true;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 12F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(624, 398);
            this.Controls.Add(this.comboBox1);
            this.Controls.Add(this.button3);
            this.Controls.Add(this.button2);
            this.Controls.Add(this.另存为);
            this.Controls.Add(this.替换);
            this.Controls.Add(this.提取);
            this.Controls.Add(this.保存);
            this.Controls.Add(this.button1);
            this.Controls.Add(this.浏览器);
            this.Name = "B浏览器";
            this.Text = "B浏览器";
            this.Load += new System.EventHandler(this.B浏览器_Load);
            this.DragDrop += new System.Windows.Forms.DragEventHandler(this.拖放文件);
            this.DragEnter += new System.Windows.Forms.DragEventHandler(this.拖放开始);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.ListView 浏览器;
        private System.Windows.Forms.ColumnHeader 序号;
        private System.Windows.Forms.ColumnHeader 文件名;
        private System.Windows.Forms.ColumnHeader 类型;
        private System.Windows.Forms.ColumnHeader 大小;
        private System.Windows.Forms.Button button1;
        private System.Windows.Forms.OpenFileDialog 打开文件;
        private System.Windows.Forms.SaveFileDialog 保存文件;
        private System.Windows.Forms.Button 保存;
        private System.Windows.Forms.Button 提取;
        private System.Windows.Forms.Button 替换;
        private System.Windows.Forms.Button 另存为;
        private System.Windows.Forms.Button button2;
        private System.Windows.Forms.Button button3;
        private System.Windows.Forms.ComboBox comboBox1;
    }
}

