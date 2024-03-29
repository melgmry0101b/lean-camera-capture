using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace LeanCameraCapture.Demos.WinForms
{
    public partial class MainForm : Form
    {
        public MainForm()
        {
            InitializeComponent();
        }

        private void uiBtnLoad_Click(object sender, EventArgs e)
        {
            uiCmbDevices.DataSource = null;

            ReadOnlyCollection<CameraCaptureDevice> captureDevices;
            try
            {
                captureDevices = CameraCaptureDevice.GetCameraCaptureDevices();
            }
            catch (CameraCaptureException ex)
            {
                MessageBox.Show(
                    "Error occurred during GetCameraCaptureDevices().\r\n" + ex.ToString(),
                    "Error",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
                return;
            }

            uiCmbDevices.DisplayMember = "DeviceName";
            uiCmbDevices.DataSource = captureDevices;
        }
    }
}
