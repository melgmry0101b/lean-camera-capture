using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace LeanCameraCapture.Demos.WinForms
{
    static class Program
    {
        /// <summary>
        ///  The main entry point for the application.
        /// </summary>
        [STAThread]
        private static void Main()
        {
            try
            {
                CameraCaptureManager.Start();
            }
            catch (CameraCaptureException ex)
            {
                MessageBox.Show(
                    "Error occurred starting CameraCaptureManager.\r\n" + ex.ToString(),
                    "Error",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
                return;
            }

            Application.ApplicationExit += Application_ApplicationExit;

            Application.SetHighDpiMode(HighDpiMode.SystemAware);
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new MainForm());
        }

        private static void Application_ApplicationExit(object sender, EventArgs e)
        {
            try
            {
                CameraCaptureManager.Stop();
            }
            catch (CameraCaptureException ex)
            {
                MessageBox.Show(
                    "Error occurred stopping CameraCaptureManager.\r\n" + ex.ToString(),
                    "Error",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
                return;
            }
        }
    }
}
