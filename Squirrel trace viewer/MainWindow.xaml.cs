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
        List<Instruction> instructionsList;
        List<Instruction> searchBoxMatches;
        int searchBoxMatchesPos;

        public MainWindow()
        {
            InitializeComponent();
            instructionsList = new List<Instruction>();
            searchBoxMatches = new List<Instruction>();
            searchBoxMatchesPos = 0;
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
            instructionsList = new List<Instruction>();
            Dictionary<UInt32, AElement> objects = new Dictionary<uint, AElement>();
            foreach (JObject it in root)
            {
                if ((string)it["type"] == "instruction")
                {
                    Instruction instruction = new Instruction(it);
                    instruction.Load(objects);
                    instructionsList.Add(instruction);
                }
                else if ((string)it["type"] == "object")
                {
                    UInt32 addr = AObject.StrToAddr((string)it["address"]);
                    JToken content = it["content"];
                    if (content != null)
                        objects[addr] = AElement.Create(content);
                    else
                    {
                        //Console.WriteLine("Invalid object " + addr + ": content is null.");
                        objects[addr] = new Null();
                    }
                }
            }

            grid.ItemsSource = instructionsList;
            return true;
        }

        private void grid_SelectedCellsChanged(object sender, SelectedCellsChangedEventArgs e)
        {
            AElement obj = grid.SelectedItem as AElement;
            if (obj == null)
                return;
            tree.Items.Clear();
            obj.AddChildsToTree(tree.Items);
            foreach (var it in tree.Items)
                ExpandAll(it as TreeViewItem);
        }

        private void TreeViewItem_MouseRightButtonUp(object sender, MouseButtonEventArgs e)
        {
            (e.Source as TreeViewItem).Focus();
        }

        private void ExpandAll(TreeViewItem item)
        {
            if (item == null)
                return;
            item.IsExpanded = true;
            foreach (var it in item.Items)
                ExpandAll(it as TreeViewItem);
        }

        private void CollapseAll(TreeViewItem item)
        {
            if (item == null)
                return;
            foreach (var it in item.Items)
                CollapseAll(it as TreeViewItem);
            item.IsExpanded = false;
        }

        private void Generic_CanExecute(object sender, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = true;
        }

        private void CollapseAll_Executed(object sender, ExecutedRoutedEventArgs e)
        {
            CollapseAll(e.Source as TreeViewItem);
        }

        private void ExpandAll_Executed(object sender, ExecutedRoutedEventArgs e)
        {
            ExpandAll(e.Source as TreeViewItem);
        }

        private void FilterBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            string text = (sender as TextBox).Text;
            // TODO: filter in a background thread.
            grid.ItemsSource = instructionsList.Where(x => x.Contains(text));
        }

        private void selectSearchResult()
        {
            int count = searchBoxMatches.Count;
            if (searchBoxMatchesPos >= count)
                searchBoxMatchesPos = count - 1;
            if (searchBoxMatchesPos < 0)
                searchBoxMatchesPos = 0;
            if (count != 0)
            {
                Instruction i = searchBoxMatches[searchBoxMatchesPos];
                grid.SelectedItem = i;
                grid.ScrollIntoView(i);
            }
            if (searchBoxMatchesPos > 0)
                search_prev.IsEnabled = true;
            else
                search_prev.IsEnabled = false;
            if (searchBoxMatchesPos < count - 1)
                search_next.IsEnabled = true;
            else
                search_next.IsEnabled = false;
        }

        private void SearchBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            string text = (sender as TextBox).Text;
            // TODO: filter in a background thread.
            searchBoxMatches = (grid.ItemsSource as IEnumerable<Instruction>).Where(x => x.Contains(text)).ToList();
            searchBoxMatchesPos = 0;
            selectSearchResult();
        }

        private void SearchBox_prev(object sender, RoutedEventArgs e)
        {
            searchBoxMatchesPos--;
            selectSearchResult();
        }

        private void SearchBox_next(object sender, RoutedEventArgs e)
        {
            searchBoxMatchesPos++;
            selectSearchResult();
        }
    }

    public static class CustomCommands
    {
        public static readonly RoutedUICommand ExpandAll = new RoutedUICommand("Expand all", "ExpandAll", typeof(CustomCommands));
        public static readonly RoutedUICommand CollapseAll = new RoutedUICommand("Collapse all", "CollapseAll", typeof(CustomCommands));
    }
}
