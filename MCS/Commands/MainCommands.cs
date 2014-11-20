using MCS.MainWindows;
using System;
using System.Collections.Generic;
using System.Windows;
using System.Windows.Input;

namespace MCS.Commands
{
    public class MainCommands
    {
        
        public ICommand LogWindowCommand
        {
            get { return new DelegateCommand((o) => { WindowsManager.ShowWindow(typeof(LogWindow), o); }); }
        }


        public ICommand ContentWindowCommand
        {
            get { return new DelegateCommand((o) => { WindowsManager.ShowWindow(typeof(ContentWindow), o); }); }
        }

    }
}
