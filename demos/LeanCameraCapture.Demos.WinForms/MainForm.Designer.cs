
namespace LeanCameraCapture.Demos.WinForms
{
    partial class MainForm
    {
        /// <summary>
        ///  Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        ///  Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        ///  Required method for Designer support - do not modify
        ///  the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.uiBtnLoad = new System.Windows.Forms.Button();
            this.uiCmbDevices = new System.Windows.Forms.ComboBox();
            this.uiLblDevicesHeader = new System.Windows.Forms.Label();
            this.SuspendLayout();
            // 
            // uiBtnLoad
            // 
            this.uiBtnLoad.Location = new System.Drawing.Point(376, 33);
            this.uiBtnLoad.Name = "uiBtnLoad";
            this.uiBtnLoad.Size = new System.Drawing.Size(114, 23);
            this.uiBtnLoad.TabIndex = 0;
            this.uiBtnLoad.Text = "Load Devices";
            this.uiBtnLoad.UseVisualStyleBackColor = true;
            this.uiBtnLoad.Click += new System.EventHandler(this.uiBtnLoad_Click);
            // 
            // uiCmbDevices
            // 
            this.uiCmbDevices.FormattingEnabled = true;
            this.uiCmbDevices.Location = new System.Drawing.Point(12, 33);
            this.uiCmbDevices.Name = "uiCmbDevices";
            this.uiCmbDevices.Size = new System.Drawing.Size(358, 23);
            this.uiCmbDevices.TabIndex = 1;
            // 
            // uiLblDevicesHeader
            // 
            this.uiLblDevicesHeader.AutoSize = true;
            this.uiLblDevicesHeader.Font = new System.Drawing.Font("Segoe UI", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point);
            this.uiLblDevicesHeader.ForeColor = System.Drawing.Color.SteelBlue;
            this.uiLblDevicesHeader.Location = new System.Drawing.Point(12, 9);
            this.uiLblDevicesHeader.Name = "uiLblDevicesHeader";
            this.uiLblDevicesHeader.Size = new System.Drawing.Size(250, 21);
            this.uiLblDevicesHeader.TabIndex = 2;
            this.uiLblDevicesHeader.Text = "Available Camera Capture Devices:";
            // 
            // MainForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 15F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.White;
            this.ClientSize = new System.Drawing.Size(502, 450);
            this.Controls.Add(this.uiLblDevicesHeader);
            this.Controls.Add(this.uiCmbDevices);
            this.Controls.Add(this.uiBtnLoad);
            this.Name = "MainForm";
            this.Text = "LeanCameraCapture WinForms Demo";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button uiBtnLoad;
        private System.Windows.Forms.ComboBox uiCmbDevices;
        private System.Windows.Forms.Label uiLblDevicesHeader;
    }
}

