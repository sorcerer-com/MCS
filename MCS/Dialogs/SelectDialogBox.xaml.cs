using MCS.Managers;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Windows;
using System.Windows.Input;

namespace MCS.Dialogs
{
    /// <summary>
    /// Interaction logic for SelectDialogBox.xaml
    /// </summary>
    public partial class SelectDialogBox : Window
    {
        public string Label { get; set; }

        public ObservableCollection<string> Items { get; private set; }

        public string Text { get; set; }

        #region Commands

        public ICommand OkButtonCommand
        {
            get { return new DelegateCommand((o) => { this.DialogResult = true; }); }
        }

        public ICommand CancelButtonCommand
        {
            get { return new DelegateCommand((o) => { this.DialogResult = false; }); }
        }

        #endregion


        public SelectDialogBox()
        {
            InitializeComponent();
            this.DataContext = this;

            this.Items = new ObservableCollection<string>();
        }

        public SelectDialogBox(string title, string label, List<string> items)
            : this()
        {
            this.Title = title;
            this.Label = label + ":";
            this.Items = new ObservableCollection<string>(items);
            if (items.Count > 0)
                this.Text = items[0];
        }

        public SelectDialogBox(string title, string label, List<string> items, string text)
            : this(title, label, items)
        {
            this.Text = text;
        }


        private void SelectDialogBox_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
                this.OkButtonCommand.Execute(null);
            else if (e.Key == Key.Escape)
                this.CancelButtonCommand.Execute(null);
        }


        public static string Show(string title, string label, List<string> items)
        {
            string text = items.Count > 0 ? items[0] : "";
            return SelectDialogBox.Show(title, label, items, text);
        }

        public static string Show(string title, string label, List<string> items, string text)
        {
            SelectDialogBox dialog = new SelectDialogBox(title, label, items, text);

            // show dialog where the mouse is
            Window mainWindow = Application.Current.MainWindow;
            Point mousePos = Mouse.GetPosition(mainWindow);
            mousePos = mainWindow.PointToScreen(mousePos);
            dialog.Left = mousePos.X;
            dialog.Top = mousePos.Y;

            bool? result = dialog.ShowDialog();
            if (result == true)
                return dialog.Text;
            else if (result == false)
                return string.Empty;
            return null;
        }
    }
}
