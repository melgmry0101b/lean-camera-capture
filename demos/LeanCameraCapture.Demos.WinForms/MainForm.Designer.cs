
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
            this.UiBtnLoad = new System.Windows.Forms.Button();
            this.UiCmbDevices = new System.Windows.Forms.ComboBox();
            this.UiLblDevicesHeader = new System.Windows.Forms.Label();
            this.UiPicViewer = new System.Windows.Forms.PictureBox();
            this.UiGrpViewer = new System.Windows.Forms.GroupBox();
            this.UiBtnSaveStill = new System.Windows.Forms.Button();
            this.UiBtnStopCapture = new System.Windows.Forms.Button();
            this.UiBtnStartCapture = new System.Windows.Forms.Button();
            ((System.ComponentModel.ISupportInitialize)(this.UiPicViewer)).BeginInit();
            this.UiGrpViewer.SuspendLayout();
            this.SuspendLayout();
            // 
            // UiBtnLoad
            // 
            this.UiBtnLoad.Location = new System.Drawing.Point(504, 33);
            this.UiBtnLoad.Name = "UiBtnLoad";
            this.UiBtnLoad.Size = new System.Drawing.Size(114, 23);
            this.UiBtnLoad.TabIndex = 0;
            this.UiBtnLoad.Text = "Load Devices";
            this.UiBtnLoad.UseVisualStyleBackColor = true;
            this.UiBtnLoad.Click += new System.EventHandler(this.UiBtnLoad_Click);
            // 
            // UiCmbDevices
            // 
            this.UiCmbDevices.FormattingEnabled = true;
            this.UiCmbDevices.Location = new System.Drawing.Point(12, 33);
            this.UiCmbDevices.Name = "UiCmbDevices";
            this.UiCmbDevices.Size = new System.Drawing.Size(486, 23);
            this.UiCmbDevices.TabIndex = 1;
            // 
            // UiLblDevicesHeader
            // 
            this.UiLblDevicesHeader.AutoSize = true;
            this.UiLblDevicesHeader.Font = new System.Drawing.Font("Segoe UI", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point);
            this.UiLblDevicesHeader.ForeColor = System.Drawing.Color.SteelBlue;
            this.UiLblDevicesHeader.Location = new System.Drawing.Point(12, 9);
            this.UiLblDevicesHeader.Name = "UiLblDevicesHeader";
            this.UiLblDevicesHeader.Size = new System.Drawing.Size(250, 21);
            this.UiLblDevicesHeader.TabIndex = 2;
            this.UiLblDevicesHeader.Text = "Available Camera Capture Devices:";
            // 
            // UiPicViewer
            // 
            this.UiPicViewer.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.UiPicViewer.Location = new System.Drawing.Point(6, 22);
            this.UiPicViewer.Name = "UiPicViewer";
            this.UiPicViewer.Size = new System.Drawing.Size(480, 360);
            this.UiPicViewer.TabIndex = 3;
            this.UiPicViewer.TabStop = false;
            // 
            // UiGrpViewer
            // 
            this.UiGrpViewer.Controls.Add(this.UiBtnSaveStill);
            this.UiGrpViewer.Controls.Add(this.UiBtnStopCapture);
            this.UiGrpViewer.Controls.Add(this.UiBtnStartCapture);
            this.UiGrpViewer.Controls.Add(this.UiPicViewer);
            this.UiGrpViewer.Location = new System.Drawing.Point(12, 62);
            this.UiGrpViewer.Name = "UiGrpViewer";
            this.UiGrpViewer.Size = new System.Drawing.Size(614, 395);
            this.UiGrpViewer.TabIndex = 4;
            this.UiGrpViewer.TabStop = false;
            this.UiGrpViewer.Text = "Viewport";
            // 
            // UiBtnSaveStill
            // 
            this.UiBtnSaveStill.Location = new System.Drawing.Point(492, 80);
            this.UiBtnSaveStill.Name = "UiBtnSaveStill";
            this.UiBtnSaveStill.Size = new System.Drawing.Size(114, 23);
            this.UiBtnSaveStill.TabIndex = 7;
            this.UiBtnSaveStill.Text = "Save Still";
            this.UiBtnSaveStill.UseVisualStyleBackColor = true;
            // 
            // UiBtnStopCapture
            // 
            this.UiBtnStopCapture.Location = new System.Drawing.Point(492, 51);
            this.UiBtnStopCapture.Name = "UiBtnStopCapture";
            this.UiBtnStopCapture.Size = new System.Drawing.Size(114, 23);
            this.UiBtnStopCapture.TabIndex = 6;
            this.UiBtnStopCapture.Text = "Stop Capture";
            this.UiBtnStopCapture.UseVisualStyleBackColor = true;
            this.UiBtnStopCapture.Click += new System.EventHandler(this.UiBtnStopCapture_Click);
            // 
            // UiBtnStartCapture
            // 
            this.UiBtnStartCapture.Location = new System.Drawing.Point(492, 22);
            this.UiBtnStartCapture.Name = "UiBtnStartCapture";
            this.UiBtnStartCapture.Size = new System.Drawing.Size(114, 23);
            this.UiBtnStartCapture.TabIndex = 5;
            this.UiBtnStartCapture.Text = "Start Capture";
            this.UiBtnStartCapture.UseVisualStyleBackColor = true;
            this.UiBtnStartCapture.Click += new System.EventHandler(this.UiBtnStartCapture_Click);
            // 
            // MainForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 15F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.White;
            this.ClientSize = new System.Drawing.Size(634, 467);
            this.Controls.Add(this.UiGrpViewer);
            this.Controls.Add(this.UiLblDevicesHeader);
            this.Controls.Add(this.UiCmbDevices);
            this.Controls.Add(this.UiBtnLoad);
            this.Name = "MainForm";
            this.Text = "LeanCameraCapture WinForms Demo";
            ((System.ComponentModel.ISupportInitialize)(this.UiPicViewer)).EndInit();
            this.UiGrpViewer.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button UiBtnLoad;
        private System.Windows.Forms.ComboBox UiCmbDevices;
        private System.Windows.Forms.Label UiLblDevicesHeader;
        private System.Windows.Forms.PictureBox UiPicViewer;
        private System.Windows.Forms.GroupBox UiGrpViewer;
        private System.Windows.Forms.Button UiBtnSaveStill;
        private System.Windows.Forms.Button UiBtnStopCapture;
        private System.Windows.Forms.Button UiBtnStartCapture;
    }
}

