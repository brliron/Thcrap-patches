using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace Squirrel_trace_viewer
{
    /// <summary>
    /// Logique d'interaction pour MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
        }

        private void Load_json(object sender, RoutedEventArgs e)
        {
            OpenFileDialog dialog = new OpenFileDialog();
            if (dialog.ShowDialog(this) != true)
                return;
            if (!CloseJsonFile(dialog.FileName))
                return;
            ParseJson(dialog.FileName);
        }

        private bool CloseJsonFile(string fn)
        {
            FileStream stream = new FileStream(fn, FileMode.Open, FileAccess.ReadWrite);
            long read_size = stream.Length > 8 ? 8 : stream.Length;
            stream.Seek(-read_size, SeekOrigin.End);
            byte[] bytes = new byte[read_size];
            stream.Read(bytes, 0, (int)read_size);
            for (long i = read_size - 1; i >= 0; i--)
            {
                if (bytes[i] == ']')
                {
                    // Valid array
                    stream.Dispose();
                    return true;
                }
                else if (bytes[i] == ',')
                {
                    // Close the array
                    bytes[i] = (byte)']';
                    stream.Seek(-read_size, SeekOrigin.End);
                    stream.Write(bytes, 0, (int)read_size);
                    stream.Dispose();
                    return true;
                }
            }

            MessageBox.Show(this, fn + " doesn't seem to be a JSON file created by Squirrel tracer.");
            stream.Dispose();
            return false;
        }

        private bool ParseJson(string fn)
        {
            JToken rootToken = JToken.ReadFrom(new JsonTextReader(File.OpenText(fn)));
            if (rootToken == null)
            {
                Console.WriteLine("root is null");
                return false;
            }
            if (rootToken.Type != JTokenType.Array)
            {
                MessageBox.Show(this, " isn't an array (is " + rootToken.Type + ").");
                return false;
            }

            JArray root = (JArray)rootToken;
            List<Instruction> list = new List<Instruction>();
            Dictionary<UInt32, AElement> objects = new Dictionary<uint, AElement>();
            foreach (JObject it in root)
            {
                if ((string)it["type"] == "instruction")
                {
                    list.Add(new Instruction(it, objects));
                }
                else if ((string)it["type"] == "object")
                {
                    UInt32 addr = AObject.StrToAddr((string)it["address"]);
                    objects[addr] = new BoringObject(addr, it["content"]);
                }
            }

            grid.ItemsSource = list;
            return true;
        }
    }
}
