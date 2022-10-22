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
            Application.SetHighDpiMode(HighDpiMode.SystemAware);
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);

            var mainForm = new MainForm();

            try
            {
                CameraCaptureManager.Start(mainForm.Handle);
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

            Application.Run(mainForm);
        }

        private static void Application_ApplicationExit(object? sender, EventArgs e)
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
