using MCS.Managers;
using System.Windows;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;

namespace MCS.Dialogs
{
    public enum ExtendedMessageBoxButton
    {
        OK,
        OKCancel,
        YesNoCancel,
        YesNo,
        YesYesToAllNoNoToAllCancel,
        YesYesToAllNoNoToAll,
    }

    public enum ExtendedMessageBoxImage
    {
        None,
        Error,
        Hand,
        Stop,
        Question,
        Exclamation,
        Warning,
        Information,
        Asterisk
    }

    public enum ExtendedMessageBoxResult
    {
        None,
        OK,
        Cancel,
        Yes,
        No,
        YesToAll,
        NoToAll
    }

    /// <summary>
    /// Interaction logic for ExtendedMessageBox.xaml
    /// </summary>
    public partial class ExtendedMessageBox : Window
    {
        public string Text { get; set; }

        private ExtendedMessageBoxButton button;
        public string YesButtonText
        {
            get { return button == ExtendedMessageBoxButton.OK || button == ExtendedMessageBoxButton.OKCancel ? "OK" : "Yes"; }
        }
        public bool IsYesToAllButtonVisible
        {
            get { return button == ExtendedMessageBoxButton.YesYesToAllNoNoToAll || button == ExtendedMessageBoxButton.YesYesToAllNoNoToAllCancel; }
        }
        public bool IsNoButtonVisible
        {
            get { return !(button == ExtendedMessageBoxButton.OK || button == ExtendedMessageBoxButton.OKCancel); }
        }
        public bool IsNoToAllButtonVisible
        {
            get { return button == ExtendedMessageBoxButton.YesYesToAllNoNoToAll || button == ExtendedMessageBoxButton.YesYesToAllNoNoToAllCancel; }
        }
        public bool IsCancelButtonVisible
        {
            get { return button == ExtendedMessageBoxButton.OKCancel || button == ExtendedMessageBoxButton.YesNoCancel || button == ExtendedMessageBoxButton.YesYesToAllNoNoToAllCancel; }
        }

        private ExtendedMessageBoxImage image;
        public ImageSource Image
        {
            get
            {
                System.Drawing.Icon image = null;
                switch(this.image)
                {
                    case ExtendedMessageBoxImage.Error:
                        image = System.Drawing.SystemIcons.Error;
                        break;
                    case ExtendedMessageBoxImage.Hand:
                        image = System.Drawing.SystemIcons.Hand;
                        break;
                    case ExtendedMessageBoxImage.Stop:
                        image = System.Drawing.SystemIcons.Error;
                        break;
                    case ExtendedMessageBoxImage.Question:
                        image = System.Drawing.SystemIcons.Question;
                        break;
                    case ExtendedMessageBoxImage.Exclamation:
                        image = System.Drawing.SystemIcons.Exclamation;
                        break;
                    case ExtendedMessageBoxImage.Warning:
                        image = System.Drawing.SystemIcons.Warning;
                        break;
                    case ExtendedMessageBoxImage.Information:
                        image = System.Drawing.SystemIcons.Information;
                        break;
                    case ExtendedMessageBoxImage.Asterisk:
                        image = System.Drawing.SystemIcons.Asterisk;
                        break;
                }
                return System.Windows.Interop.Imaging.CreateBitmapSourceFromHIcon(image.Handle, Int32Rect.Empty, BitmapSizeOptions.FromEmptyOptions());
            }
        }

        public ExtendedMessageBoxResult Result { get; set; }

        #region Commands

        public ICommand YesButtonCommand
        {
            get { return new DelegateCommand((o) => { if (this.YesButtonText == "OK") this.Result = ExtendedMessageBoxResult.OK; else this.Result = ExtendedMessageBoxResult.Yes; this.DialogResult = true; }); }
        }

        public ICommand YesToAllButtonCommand
        {
            get { return new DelegateCommand((o) => { this.Result = ExtendedMessageBoxResult.YesToAll; this.DialogResult = true; }); }
        }


        public ICommand NoButtonCommand
        {
            get { return new DelegateCommand((o) => { this.Result = ExtendedMessageBoxResult.No; this.DialogResult = true; }); }
        }


        public ICommand NoToAllButtonCommand
        {
            get { return new DelegateCommand((o) => { this.Result = ExtendedMessageBoxResult.NoToAll; this.DialogResult = true; }); }
        }


        public ICommand CancelButtonCommand
        {
            get { return new DelegateCommand((o) => { this.Result = ExtendedMessageBoxResult.Cancel; this.DialogResult = false; }); }
        }

        #endregion


        private ExtendedMessageBox()
        {
            InitializeComponent();
            this.DataContext = this;
        }

        public ExtendedMessageBox(string text, string title)
            : this()
        {
            this.Text = text;
            this.Title = title;
        }

        public ExtendedMessageBox(string text, string title, ExtendedMessageBoxButton button)
            : this(text, title)
        {
            this.button = button;
        }

        public ExtendedMessageBox(string text, string title, ExtendedMessageBoxButton button, ExtendedMessageBoxImage icon)
            : this(text, title, button)
        {
            this.image = icon;
        }


        public static ExtendedMessageBoxResult Show(string text, string title)
        {
            return ExtendedMessageBox.Show(text, title, ExtendedMessageBoxButton.OK, ExtendedMessageBoxImage.None);
        }

        public static ExtendedMessageBoxResult Show(string text, string title, ExtendedMessageBoxButton button)
        {
            return ExtendedMessageBox.Show(text, title, button, ExtendedMessageBoxImage.None);
        }

        public static ExtendedMessageBoxResult Show(string text, string title, ExtendedMessageBoxButton button, ExtendedMessageBoxImage icon)
        {
            ExtendedMessageBox dialog = new ExtendedMessageBox(text, title, button, icon);
            dialog.WindowStartupLocation = WindowStartupLocation.CenterOwner;

            dialog.ShowDialog();
            return dialog.Result;
        }
    }
}
