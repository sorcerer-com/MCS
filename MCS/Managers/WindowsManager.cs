using MCS.Dialogs;
using MCS.MainWindows;
using MyEngine;
using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Windows;
using System.Windows.Input;
using System.Xml;

namespace MCS.Managers
{
    public static class WindowsManager
    {
        private struct HotKeyInfo
        {
            public Key Key { get; set; }
            public bool Ctrl { get; set; }
            public bool Alt { get; set; }
            public bool Shift { get; set; }

            public string SourceType { get; set; }

            public string CommandName { get; set; }

            public HotKeyInfo(Key key, bool ctrl, bool alt, bool shift, string sourceType, string commandName)
                : this()
            {
                this.Key = key;
                this.Ctrl = ctrl;
                this.Alt = alt;
                this.Shift = shift;
                this.SourceType = sourceType;
                this.CommandName = commandName;
            }
        }


        private static Dictionary<string, Window> windows = new Dictionary<string, Window>();
        private static Dictionary<Type, List<HotKeyInfo>> hotkeys = new Dictionary<Type, List<HotKeyInfo>>();
        

        public static void ShowWindow(Type windowType, Object param = null)
        {
            if (!windowType.IsSubclassOf(typeof(Window)))
                return;

            if (!windows.ContainsKey(windowType.Name))
                windows.Add(windowType.Name, null);

            Window window = windows[windowType.Name];
            if (window != null && window.IsVisible)
            {
                window.Activate();
                return;
            }
            else if (window != null)
                window.Close();

            try
            {
                if (param != null)
                    window = windowType.GetConstructor(new Type[] { param.GetType() }).Invoke(new object[] { param }) as Window;
                else
                    window = windowType.GetConstructor(Type.EmptyTypes).Invoke(null) as Window;
            }
            catch { }
            if (window == null)
                throw new Exception("Cannot construct window from type: " + windowType + " with parametar: " + param);

            window.Owner = Application.Current.MainWindow;
            window.KeyDown += Window_KeyDown;
            window.Closing += (sender, e) => { window.KeyDown -= Window_KeyDown; };

            window.Show();
            windows[windowType.Name] = window;
        }

        public static void CloseAllWindows()
        {
            foreach (var pair in windows)
                pair.Value.Close();
        }


        // Hot keys functionality
        public static void LoadConfig(XmlDocument xmlDoc)
        {
            XmlNodeList xmlNodes = xmlDoc.GetElementsByTagName("Hotkeys");
            if (xmlNodes.Count == 0)
            {
                MEngine.Log(ELogType.Warning, "Editor", "Hotkeys cannot be loaded from config file");
                defaultHotkeys();
                return;
            }

            XmlNode xmlRoot = xmlNodes.Item(0);
            for (int i = 0; i < xmlRoot.ChildNodes.Count; i++)
            {
                XmlNode xmlType = xmlRoot.ChildNodes.Item(i);
                List<HotKeyInfo> keys = new List<HotKeyInfo>();

                for (int j = 0; j < xmlType.ChildNodes.Count; j++)
                {
                    XmlNode xmlKey = xmlType.ChildNodes.Item(j);
                    string key = xmlKey.Attributes.GetNamedItem("Key").Value;
                    string ctrl = xmlKey.Attributes.GetNamedItem("Ctrl").Value;
                    string alt = xmlKey.Attributes.GetNamedItem("Alt").Value;
                    string shift = xmlKey.Attributes.GetNamedItem("Shift").Value;
                    string sourceType = xmlKey.Attributes.GetNamedItem("SourceType").Value;
                    string commandName = xmlKey.Attributes.GetNamedItem("Command").Value;

                    keys.Add(new HotKeyInfo((Key)Enum.Parse(typeof(Key), key), bool.Parse(ctrl), bool.Parse(alt), bool.Parse(shift), sourceType, commandName));
                }

                hotkeys.Add(Type.GetType(xmlType.Name), keys);
            }
        }

        private static void defaultHotkeys()
        {
            List<HotKeyInfo> keys = null;

            // MainWindow
            keys = new List<HotKeyInfo>();
            keys.Add(new HotKeyInfo(Key.N, true, false, false, "", "NewSceneCommand"));
            keys.Add(new HotKeyInfo(Key.O, true, false, false, "", "OpenSceneCommand"));
            keys.Add(new HotKeyInfo(Key.S, true, false, false, "", "SaveSceneCommand"));
            keys.Add(new HotKeyInfo(Key.F8, false, false, false, "", "LogWindowCommand"));
            keys.Add(new HotKeyInfo(Key.F3, false, false, false, "", "ContentWindowCommand"));
            keys.Add(new HotKeyInfo(Key.F, true, false, false, "", "FindWindowCommand"));
            keys.Add(new HotKeyInfo(Key.E, true, false, false, "", "EnvironmentWindowCommand"));
            keys.Add(new HotKeyInfo(Key.L, true, false, false, "", "LayersWindowCommand"));
            keys.Add(new HotKeyInfo(Key.F9, false, false, false, "", "RenderWindowCommand"));
            hotkeys.Add(typeof(MainWindow), keys);

            // ContentWindow
            keys = new List<HotKeyInfo>();
            keys.Add(new HotKeyInfo(Key.F2, false, false, false, "TreeView", "RenamePathCommand"));
            keys.Add(new HotKeyInfo(Key.Delete, false, false, false, "TreeView", "DeletePathCommand"));

            keys.Add(new HotKeyInfo(Key.F3, false, false, false, "ListView", "CloneElementCommand"));
            keys.Add(new HotKeyInfo(Key.F2, false, false, false, "ListView", "RenameElementCommand"));
            keys.Add(new HotKeyInfo(Key.F4, false, false, false, "ListView", "MoveElementCommand"));
            keys.Add(new HotKeyInfo(Key.Delete, false, false, false, "ListView", "DeleteElementCommand"));
            keys.Add(new HotKeyInfo(Key.F9, false, false, false, "ListView", "ExportElementCommand"));
            hotkeys.Add(typeof(ContentWindow), keys);

            // RenderWindow
            keys = new List<HotKeyInfo>();
            keys.Add(new HotKeyInfo(Key.F9, false, false, false, "", "RenderCommand"));
            keys.Add(new HotKeyInfo(Key.F11, false, false, false, "", "SaveBufferCommand"));
            keys.Add(new HotKeyInfo(Key.F11, true, false, false, "", "SaveBufferCommand"));
            hotkeys.Add(typeof(RenderWindow), keys);

            saveHotkeys();
        }

        private static void saveHotkeys()
        {
            XmlDocument xmlDoc = ConfigManager.XmlDoc;
            XmlNodeList xmlNodes = xmlDoc.GetElementsByTagName("Hotkeys");
            XmlElement xmlRoot = null;
            if (xmlNodes.Count == 0)
            {
                xmlRoot = xmlDoc.CreateElement("Hotkeys");
                xmlDoc.DocumentElement.AppendChild(xmlRoot);
            }
            else
                xmlRoot = xmlNodes.Item(0) as XmlElement;
            xmlRoot.RemoveAll();

            foreach(var pair in hotkeys)
            {
                XmlElement xmlType = xmlDoc.CreateElement(pair.Key.FullName);
                xmlRoot.AppendChild(xmlType);

                foreach(var info in pair.Value)
                {
                    XmlElement xmlKey = xmlDoc.CreateElement("Hotkey");
                    xmlKey.SetAttribute("Key", info.Key.ToString());
                    xmlKey.SetAttribute("Ctrl", info.Ctrl.ToString());
                    xmlKey.SetAttribute("Alt", info.Alt.ToString());
                    xmlKey.SetAttribute("Shift", info.Shift.ToString());
                    xmlKey.SetAttribute("SourceType", info.SourceType);
                    xmlKey.SetAttribute("Command", info.CommandName);
                    xmlType.AppendChild(xmlKey);
                }
            }

            ConfigManager.SaveConfig();
        }

        public static string GetHotkey(Type windowType, string commandName, bool brackets = false)
        {
            if (!hotkeys.ContainsKey(windowType))
                return null;

            List<HotKeyInfo> keys = hotkeys[windowType];
            foreach (HotKeyInfo info in keys)
            {
                if (info.CommandName == commandName)
                {
                    string hotkey = string.Empty;
                    if (info.Ctrl)
                        hotkey += "Ctrl+";
                    if (info.Alt)
                        hotkey += "Alt+";
                    if (info.Shift)
                        hotkey += "Shift+";
                    hotkey += info.Key.ToString();
                    if (brackets)
                        return "(" + hotkey + ")";
                    else
                        return hotkey;
                }
            }

            return null;
        }


        public static void Window_KeyDown(object sender, KeyEventArgs e)
        {
            Type type = sender.GetType();
            if (!hotkeys.ContainsKey(type))
                return;

            List<HotKeyInfo> keys = hotkeys[type];
            foreach (var info in keys)
            {
                bool ctrl = Keyboard.IsKeyDown(Key.LeftCtrl) || Keyboard.IsKeyDown(Key.RightCtrl);
                bool alt = Keyboard.IsKeyDown(Key.LeftAlt) || Keyboard.IsKeyDown(Key.RightAlt);
                bool shift = Keyboard.IsKeyDown(Key.LeftShift) || Keyboard.IsKeyDown(Key.RightShift);
                if (info.Key != e.Key || ctrl != info.Ctrl || alt != info.Alt || shift != info.Shift)
                    continue;

                if (!string.IsNullOrEmpty(info.SourceType) && !e.Source.GetType().ToString().ToLowerInvariant().Contains(info.SourceType.ToLowerInvariant()))
                    continue;

                PropertyInfo pi = type.GetProperty(info.CommandName);
                if (pi == null)
                {
                    MEngine.Log(ELogType.Warning, "Editor", "Try to execute invalid command '" + info.CommandName + "' associate with hotkey '" + e.Key + "'");
                    continue;
                }

                ICommand command = pi.GetValue(sender) as ICommand;
                if (command == null)
                {
                    MEngine.Log(ELogType.Warning, "Editor", "Try to execute invalid command '" + info.CommandName + "' associate with hotkey '" + e.Key + "'");
                    continue;
                }

                command.Execute(null);
                e.Handled = true;
            }
        }

    }
}
