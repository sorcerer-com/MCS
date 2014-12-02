using MCS.Managers;
using System.Windows;
using System.Windows.Input;

namespace MCS.Dialogs
{
    /// <summary>
    /// Interaction logic for TextDialogBox.xaml
    /// </summary>
    public partial class TextDialogBox : Window
    {
        public string Label { get; set; }

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


        private TextDialogBox()
        {
            InitializeComponent();
            this.DataContext = this;
        }

        public TextDialogBox(string title, string label)
            : this()
        {
            this.Title = title + ":";
            this.Label = label;
        }

        public TextDialogBox(string title, string label, string text)
            : this(title, label)
        {
            this.Text = text;
        }


        private void TextBox_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
                this.OkButtonCommand.Execute(null);
            else if (e.Key == Key.Escape)
                this.CancelButtonCommand.Execute(null);
        }


        public static string Show(string title, string label, string text)
        {
            TextDialogBox dialog = new TextDialogBox(title, label, text);

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

        public static string Show(string title, string label)
        {
            return TextDialogBox.Show(title, label, "");
        }

    }
}
