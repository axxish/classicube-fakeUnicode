using System;
using System.IO;  
using System.Text;
using MCGalaxy;
using MCGalaxy.Events;
using MCGalaxy.Events.PlayerEvents;
using MCGalaxy.Events.ServerEvents;
using MCGalaxy.Network;

//using MCGalaxy.Network.Player.Networking;
//using MCGalaxy.Network.СlassicProtocol;
namespace yaoi
{
    public class yaoi : Plugin
    {
        public override string creator { get { return "crush_"; } }
        public override string MCGalaxy_Version { get { return "1.9.3.1"; } }
        public override string name { get { return "classilays"; } }

        string handshakeString { get { return "yaoi"; } }

        string layDir { get { return "layouts"; } }
        string layPath { get { return layDir+"/"+"global.cfg"; } }
        public byte handshakeChannel = 255;

        public override void Load(bool startup)
        {
            OnPluginMessageReceivedEvent.Register(Handshake, Priority.High);

            if (!Directory.Exists(layDir)) {
                    Directory.CreateDirectory(layDir);
                }

            if (!File.Exists(layPath)) {
                File.Create(layPath);
            }

        }

        public override void Unload(bool shutdown)
        {
            OnPluginMessageReceivedEvent.Unregister(Handshake);

        }

        char unicodify(int c){
            return ((char)c).Cp437ToUnicode();
        }

        void stringBuilderSend(Player p, StringBuilder lang1){
            byte[] test2 = Encoding.GetEncoding("437").GetBytes(lang1.ToString());
            byte[] msg2 = new byte[64];
            for (int i = 0; i < 64; i++){
                if(i < test2.Length) {msg2[i] = test2[i];}
                else{
                    msg2[i] = 0;
                }          
            }
            
            p.Send(Packet.PluginMessage(255, msg2));
        }

        StringBuilder parseConfigLine(Player p, string line, int num){
            StringBuilder parsed = new StringBuilder(5);

            string[] x = line.Split('=', 2, System.StringSplitOptions.None);
            //left part
            string left = x[0];
            left = left.Trim();
            if(left.Length > 1){
                parsed.Append("ClassilaysServer: malformed input at line ");
                parsed.Append(num.ToString());
                p.Message(parsed.ToString());
                return new StringBuilder(5);
            }
            else{
                byte[] leftBytes = Encoding.GetEncoding(437).GetBytes(left);
                if((leftBytes[0] >= 65 && leftBytes[0] <= 90) || (leftBytes[0] >= 48 && leftBytes[0] <= 57)){
                    parsed.Append(left);
                }
                else{
                    switch(left[0]){
                        case '-':
                            parsed.Append(((char)26).Cp437ToUnicode());
                            break;
                        case '+':
                            parsed.Append(((char)27).Cp437ToUnicode());
                            break;
                        case '[':
                            parsed.Append(((char)28).Cp437ToUnicode());
                            break;
                        case ']':
                            parsed.Append(((char)29).Cp437ToUnicode());
                            break;
                        case '/':
                            parsed.Append(((char)30).Cp437ToUnicode());
                            break;
                        case ';':
                            parsed.Append(((char)31).Cp437ToUnicode());
                            break;
                        case '\'':
                            parsed.Append(((char)32).Cp437ToUnicode());
                            break;
                        case ',':
                            parsed.Append(((char)33).Cp437ToUnicode());
                            break;
                        case '.':
                            parsed.Append(((char)34).Cp437ToUnicode());
                            break;
                        case '\\':
                            parsed.Append(((char)35).Cp437ToUnicode());
                            break;
                        case '~':
                            parsed.Append(((char)25).Cp437ToUnicode());
                            break;
                        default:
                            parsed.Append("ClassilaysServer: Malformed input at line ");
                            parsed.Append(num.ToString());
                            p.Message(parsed.ToString());
                            return new StringBuilder(5);
                    }
                }
                //right part
                string right = x[1];
                right = right.Remove(0, 1);
                string[] y = right.Split(' ', 4, System.StringSplitOptions.None);
                foreach(string val in y){
                    int charNum = Int32.Parse(val);
                    parsed.Append(((char)charNum).Cp437ToUnicode());
                }
            }
            return parsed;
        }

        void sendLays(Player p){
            StringBuilder lang1 = new StringBuilder(64);
            int lines = 0;
            foreach (string line in System.IO.File.ReadLines(layPath)) 
            {  
                lines++;
                if(line.Contains('*')){
                    if((lang1.Length+4) > 64){
                        stringBuilderSend(p, lang1);
                        lang1 = new StringBuilder(64);
                    }
                    int index = line.IndexOf('*');
                    lang1.Append(new char[]{line[index], line[index+1], line[index+2], line[index+3]});
                }
                else if(line.Contains('=')){
                    if((lang1.Length+5) > 64){
                        stringBuilderSend(p, lang1);
                        lang1 = new StringBuilder(64);
                    }
                    lang1.Append(parseConfigLine(p, line, lines));
                }
            }     

            if((lang1.Length+1) > 64){
                stringBuilderSend(p, lang1);
                lang1 = new StringBuilder(64);
            }   
            lang1.Append((((char)255).Cp437ToUnicode()));
            stringBuilderSend(p, lang1);
        }

        async void Handshake(Player p, byte channel, byte[] data)
        {
            char[] theYaoi = new char[4]; 
            for (int i = 0; i < 4; i++)
            {
                theYaoi[i] = ((char)data[i]).Cp437ToUnicode();
            }
			string test = new string(theYaoi);

			if((channel==255)&&(test == handshakeString)){
                byte[] msg = new byte[64];
                for (int i = 0; i < 64; i++)
                {
                    if(i<4) {msg[i] = data[i];}
                    else{
                        msg[i] = 0;
                    }                    
                }
                p.Send(Packet.PluginMessage(255, msg));

                
                sendLays(p);
                
			}

        }
    }
}