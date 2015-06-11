using MyEngine;
using System;
using System.Collections.Generic;
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
using System.Windows.Shapes;

namespace MCS.MainWindows
{
    /// <summary>
    /// Interaction logic for RenderWindow.xaml
    /// </summary>
    public partial class RenderWindow : Window
    {
        private MEngine engine;


        public RenderWindow(MEngine engine)
        {
            InitializeComponent();

            if (engine == null)
                throw new ArgumentNullException("engine");

            this.DataContext = this;
            this.engine = engine;
        }
    }
}
