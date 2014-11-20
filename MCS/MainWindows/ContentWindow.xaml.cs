using MEngine;
using System.Windows;

namespace MCS.MainWindows
{
    /// <summary>
    /// Interaction logic for ContentWindow.xaml
    /// </summary>
    public partial class ContentWindow : Window
    {
        public MContentManager ContentManager { get; private set; }

        public ContentWindow(MContentManager contentManager)
        {
            InitializeComponent();
            this.DataContext = this;

            this.ContentManager = contentManager;
        }
    }
}
