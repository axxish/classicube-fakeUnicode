using System;
using System.IO;
using System.Text;
using MCGalaxy;
using MCGalaxy.Events;
using MCGalaxy.Events.PlayerEvents;
using MCGalaxy.Events.ServerEvents;
using MCGalaxy.Network;

namespace fakeUnicodeServer
{
    public class fakeUnicodeServer : Plugin
    {
        public override string creator { get { return "crush_"; } }
        public override string MCGalaxy_Version { get { return "1.9.3.1"; } }
        public override string name { get { return "fakeUnicodeServer"; } }

        string handshakeString { get { return "gimme the unicode bindings"; } }
        string configurationDir { get { return "plugins/fakeUnicodeServer"; } }
        string configurationFile { get { return "bindings.cfg"; } }
        string configurationPath { get { return configurationDir + "/" + configurationFile; } }

        public byte handshakeChannel = 255;

        int[] cache;

        void error(int line)
        {
            Chat.MessageAll("fakeUnicode: Malformed input in config file!");
            Chat.MessageAll(configurationPath);
            Chat.MessageAll("on line " + line);
        }


        int[] parseCfg()
        {
            int[] message = new int[64];
            int index = 0;
            int lineCount = 0;

            foreach (string line in System.IO.File.ReadLines(configurationPath))
            {
                lineCount++;
                //p.Message(line);
                string trimmed = line.Trim();
                //for comments and empty lines
                if (!(string.IsNullOrEmpty(trimmed)))
                {
                    if (trimmed[0] == ':')
                    {
                        continue;
                    }

                }
                else
                {
                    continue;
                }

                int eqIndex = trimmed.IndexOf('=');
                if ((eqIndex == 0) || (eqIndex == -1))
                {
                    error(lineCount);
                    continue;
                }

                if (trimmed[eqIndex - 1] == '\\')
                {

                    eqIndex = trimmed.IndexOf('=', eqIndex + 1);
                    if (eqIndex == -1) error(lineCount);
                }

                //left part
                string left = trimmed.Substring(0, eqIndex - 1);
                left = left.Trim();
                if (left[0] == '\\') left = left.Remove(0, 1);

                if (left.Length > 2) error(lineCount);

                int leftAsInt = 0;
                for (int i = 0; i < left.Length; i++)
                {
                    if (Char.IsHighSurrogate(left[i]))
                    {
                        leftAsInt = Char.ConvertToUtf32(left[i], left[i + 1]);
                        i++;
                    }
                    else
                    {
                        leftAsInt = (int)left[i];
                    }
                }

                //right part
                string right = trimmed.Substring(eqIndex + 1);
                right = right.Trim();
                int rightAsInt = int.Parse(right);

                if (index >= message.Length)
                {
                    int[] temp;
                    temp = message;
                    message = new int[message.Length * 2];
                    for (int i = 0; i < temp.Length; i++)
                    {
                        message[i] = temp[i];
                    }
                }
                if (leftAsInt == 0)
                {
                    byte[] test = new byte[2];
                    test[4]=0;
                    
                    
                }
                message[index] = leftAsInt;
                message[index + 1] = rightAsInt;
                index += 2;
            }
            if (index + 1 >= message.Length)
            {
                int[] temp;
                temp = message;
                message = new int[message.Length+1];
                for (int i = 0; i < temp.Length; i++)
                {
                    message[i] = temp[i];
                }
            }
            message[index + 1] = -1;
            return message;
        }
        void sendUnicode(Player p)
        {
            byte[] data = new byte[64];
            

            for (int i = 0, j = 0; i < cache.Length; i++)
            {
                byte[] intArr = BitConverter.GetBytes(cache[i]);
                for(int z = 0; z < 4; z++){
                    if(j >= data.Length){
                        p.Send(Packet.PluginMessage(255, data));
                        data = new byte[64];
                        j = 0;
                    }
                    data[j] = intArr[z];
                    j++;
                }
            }
            p.Send(Packet.PluginMessage(255, data));
        }
        void Handshake(Player p, byte channel, byte[] data)
        {
            int length = Encoding.ASCII.GetByteCount(handshakeString);
            char[] theMessage = new char[length];

            for (int i = 0; i < length; i++)
            {
                theMessage[i] = ((char)data[i]).Cp437ToUnicode();
            }
            p.Send(Packet.PluginMessage(255, data));
            sendUnicode(p);
        }

        public override void Load(bool startup)
        {
            OnPluginMessageReceivedEvent.Register(Handshake, Priority.High);

            if (!Directory.Exists(configurationDir))
            {
                Directory.CreateDirectory(configurationDir);
            }

            if (!File.Exists(configurationPath))
            {
                File.Create(configurationPath);
            }

            cache = parseCfg();

        }

        public override void Unload(bool shutdown)
        {
            OnPluginMessageReceivedEvent.Unregister(Handshake);

        }


    }
}