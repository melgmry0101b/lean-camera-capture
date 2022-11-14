/*-----------------------------------------------------------------*\
 *
 * MainForm.cs
 *   LeanCameraCapture.Demos.WinForms
 *     lean-camera-capture
 *
 * MIT - see LICENSE at root directory
 *
 * CREATED: 2022-5-14 09:35 AM
 * AUTHORS: Mohammed Elghamry <elghamry.connect[at]outlook[dot]com>
 *
\*-----------------------------------------------------------------*/

using System;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace LeanCameraCapture.Demos.WinForms
{
    public partial class MainForm : Form
    {
        private CameraCaptureReader? m_cameraReader;

        private Bitmap? m_bitmap;

        public MainForm()
        {
            InitializeComponent();
        }

        private void UiBtnLoad_Click(object sender, EventArgs e)
        {
            UiCmbDevices.DataSource = null;

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

            UiCmbDevices.DisplayMember = "DeviceName";
            UiCmbDevices.ValueMember = "DeviceSymbolicLink";
            UiCmbDevices.DataSource = captureDevices;
        }

        private void UiBtnStartCapture_Click(object sender, EventArgs e)
        {
            // If the current reader has the same device as the selected, do nothing,
            //  else, close the current reader.
            if (m_cameraReader is not null)
            {
                if (UiCmbDevices.SelectedValue as string == m_cameraReader.Device.DeviceSymbolicLink && m_cameraReader.IsOpen)
                {
                    return;
                }

                m_cameraReader.Close();
                m_cameraReader.Dispose();
                m_cameraReader = null;
            }

            if (UiCmbDevices.SelectedItem is null) { return; }

            // Open camera reader
            try
            {
                m_cameraReader = new CameraCaptureReader(UiCmbDevices.SelectedItem as CameraCaptureDevice);
                m_cameraReader.ReadSampleSucceeded += CameraReader_ReadSampleSucceeded;
                m_cameraReader.ReadSampleFailed += CameraReader_ReadSampleFailed;

                // Open the reader
                m_cameraReader.Open();

                // Issue the first read
                m_cameraReader.ReadSample();
            }
            catch (InvalidOperationException ex)
            {
                MessageBox.Show(
                    "Error occurred during starting capture.\r\n" + ex.ToString(),
                    "Error",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
            }
            catch (CameraCaptureException ex)
            {
                MessageBox.Show(
                    $"Error occurred during starting capture. code `0x{ex.HResult:X}`\r\n" + ex.ToString(),
                    "Error",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
            }
        }

        private void UiBtnStopCapture_Click(object sender, EventArgs e)
        {
            if (m_cameraReader is not null)
            {
                m_cameraReader.Close();
                m_cameraReader.Dispose();
                m_cameraReader = null;
            }
        }

        private void CameraReader_ReadSampleFailed(object? sender, ReadSampleFailedEventArgs e)
        {
            Debug.WriteLine($"We got an error `{e.ErrorString}` with code `0x{e.HResult:X}`");

            //MessageBox.Show(
            //    $"Error during reading sample. code `0x{e.HResult:X}`\r\n" + e.ErrorString,
            //    "Error",
            //    MessageBoxButtons.OK,
            //    MessageBoxIcon.Error);

            m_cameraReader?.ReadSample();
        }

        private void CameraReader_ReadSampleSucceeded(object? sender, ReadSampleSucceededEventArgs e)
        {
            Debug.WriteLine($"We got a sample. {e.WidthInPixels}x{e.HeightInPixels}@{e.BytesPerPixel}");

            if (m_bitmap is null || m_bitmap.Width != e.WidthInPixels || m_bitmap.Height != e.HeightInPixels)
            {
                m_bitmap = new Bitmap((int)e.WidthInPixels, (int)e.HeightInPixels, PixelFormat.Format32bppRgb);
                UiPicViewer.Image = m_bitmap;
                Invoke(() => UiPicViewer.Refresh());
            }

            var bmpData = m_bitmap.LockBits(new Rectangle(0, 0, m_bitmap.Width, m_bitmap.Height), ImageLockMode.WriteOnly, m_bitmap.PixelFormat);
            Marshal.Copy(e.GetBuffer(), 0, bmpData.Scan0, bmpData.Stride * bmpData.Height);
            m_bitmap.UnlockBits(bmpData);

            Invoke(() => UiPicViewer.Refresh());

            try
            {
                m_cameraReader?.ReadSample();
            }
            catch (InvalidOperationException ex)
            {
                MessageBox.Show(
                    "Error during reading sample.\r\n" + ex.ToString(),
                    "Error",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
            }
            catch (CameraCaptureException ex)
            {
                MessageBox.Show(
                    $"Error during reading sample. code `0x{ex.HResult:X}`\r\n" + ex.ToString(),
                    "Error",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
            }
        }
    }
}
