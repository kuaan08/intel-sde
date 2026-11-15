using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Text;
using System.Windows.Forms;

namespace SdeAvxLab
{
    internal sealed class CpuProfile
    {
        public string Name;
        public string Switch;
        public string Description;
        public CpuProfile(string name, string value, string description)
        {
            Name = name; Switch = value; Description = description;
        }
        public override string ToString() { return Name; }
    }

    internal sealed class LauncherSettings
    {
        public string SdePath = "";
        public string TargetPath = "";
        public string WorkingDirectory = "";
        public string Arguments = "";
        public string Profile = "-snb";
        public string Backend = "SDE AVX Lab Runtime (self-test; não inicia jogos)";
        public bool Mix;
        public bool XInUse;

        public string Serialize()
        {
            return "SdePath=" + Escape(SdePath) + Environment.NewLine +
                   "TargetPath=" + Escape(TargetPath) + Environment.NewLine +
                   "WorkingDirectory=" + Escape(WorkingDirectory) + Environment.NewLine +
                   "Arguments=" + Escape(Arguments) + Environment.NewLine +
                   "Profile=" + Escape(Profile) + Environment.NewLine +
                   "Backend=" + Escape(Backend) + Environment.NewLine +
                   "Mix=" + Mix.ToString() + Environment.NewLine +
                   "XInUse=" + XInUse.ToString() + Environment.NewLine;
        }

        public static LauncherSettings Parse(string text)
        {
            LauncherSettings s = new LauncherSettings();
            string[] lines = text.Split(new[] { '\r', '\n' }, StringSplitOptions.RemoveEmptyEntries);
            foreach (string line in lines)
            {
                int p = line.IndexOf('=');
                if (p <= 0) continue;
                string key = line.Substring(0, p);
                string value = Unescape(line.Substring(p + 1));
                if (key == "SdePath") s.SdePath = value;
                else if (key == "TargetPath") s.TargetPath = value;
                else if (key == "WorkingDirectory") s.WorkingDirectory = value;
                else if (key == "Arguments") s.Arguments = value;
                else if (key == "Profile") s.Profile = value;
                else if (key == "Backend") s.Backend = value;
                else if (key == "Mix") bool.TryParse(value, out s.Mix);
                else if (key == "XInUse") bool.TryParse(value, out s.XInUse);
            }
            return s;
        }

        private static string Escape(string value) { return (value ?? "").Replace("\\", "\\\\").Replace("\r", "\\r").Replace("\n", "\\n"); }
        private static string Unescape(string value)
        {
            StringBuilder result = new StringBuilder(); string text = value ?? "";
            for (int i = 0; i < text.Length; i++)
            {
                if (text[i] == '\\' && i + 1 < text.Length)
                {
                    char next = text[++i]; if (next == 'n') result.Append('\n'); else if (next == 'r') result.Append('\r'); else result.Append(next);
                }
                else result.Append(text[i]);
            }
            return result.ToString();
        }
    }

    internal sealed class MainForm : Form
    {
        private readonly Color BackgroundColor = Color.FromArgb(16, 20, 28);
        private readonly Color CardColor = Color.FromArgb(27, 34, 46);
        private readonly Color FieldColor = Color.FromArgb(11, 15, 21);
        private readonly Color AccentColor = Color.FromArgb(45, 130, 220);
        private TextBox sdePath, targetPath, workPath, targetArgs;
        private ComboBox backendBox, profileBox;
        private CheckBox mixBox, xinuseBox;
        private Label backendInfo, profileInfo, status;
        private RichTextBox eventLog;
        private string appData;
        private string profileDir;
        private readonly List<CpuProfile> profiles = new List<CpuProfile>();

        public MainForm()
        {
            Text = "SDE AVX Lab — Game Compatibility Launcher";
            StartPosition = FormStartPosition.CenterScreen;
            ClientSize = new Size(940, 760);
            MinimumSize = new Size(880, 680);
            BackColor = BackgroundColor;
            ForeColor = Color.White;
            Font = new Font("Segoe UI", 9F);
            appData = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), "SDEAvxLab");
            profileDir = Path.Combine(appData, "profiles");
            Directory.CreateDirectory(profileDir);
            BuildProfiles();
            BuildUi();
            AutoDetectPaths();
            WriteLog("Aplicativo iniciado. Nenhum processo foi executado.");
        }

        private void BuildProfiles()
        {
            profiles.Add(new CpuProfile("AVX1 — Sandy Bridge (-snb)", "-snb", "Primeira referência para AVX1 e o teste do Celeron N5100."));
            profiles.Add(new CpuProfile("AVX1 — Ivy Bridge (-ivb)", "-ivb", "Perfil AVX1 alternativo para comparação."));
            profiles.Add(new CpuProfile("AVX2 — Haswell (-hsw)", "-hsw", "Perfil AVX2. Use somente se a aplicação realmente exigir AVX2."));
            profiles.Add(new CpuProfile("AVX2 — Broadwell (-bdw)", "-bdw", "Perfil AVX2 alternativo."));
            profiles.Add(new CpuProfile("AVX2 — Skylake (-skl)", "-skl", "Perfil AVX2 com recursos de Skylake."));
        }

        private Panel Card(int x, int y, int w, int h)
        {
            Panel p = new Panel(); p.BackColor = CardColor; p.Location = new Point(x, y); p.Size = new Size(w, h); return p;
        }
        private Label LabelOn(Control parent, string text, int x, int y, int w, int h, bool bold)
        {
            Label l = new Label(); l.Text = text; l.Location = new Point(x, y); l.Size = new Size(w, h);
            l.ForeColor = bold ? Color.White : Color.FromArgb(180, 191, 207);
            if (bold) l.Font = new Font("Segoe UI Semibold", 10F, FontStyle.Bold);
            parent.Controls.Add(l); return l;
        }
        private TextBox TextOn(Control parent, int x, int y, int w, int h)
        {
            TextBox t = new TextBox(); t.Location = new Point(x, y); t.Size = new Size(w, h);
            t.BackColor = FieldColor; t.ForeColor = Color.White; t.BorderStyle = BorderStyle.FixedSingle;
            parent.Controls.Add(t); return t;
        }
        private Button ButtonOn(Control parent, string text, int x, int y, int w, int h, EventHandler click, bool primary)
        {
            Button b = new Button(); b.Text = text; b.Location = new Point(x, y); b.Size = new Size(w, h);
            b.FlatStyle = FlatStyle.Flat; b.FlatAppearance.BorderSize = primary ? 0 : 1;
            b.BackColor = primary ? AccentColor : Color.FromArgb(42, 53, 69); b.ForeColor = Color.White;
            b.Click += click; parent.Controls.Add(b); return b;
        }

        private void BuildUi()
        {
            Label title = new Label(); title.Text = "SDE AVX Lab"; title.Font = new Font("Segoe UI Semibold", 24F, FontStyle.Bold);
            title.Location = new Point(28, 18); title.Size = new Size(500, 42); Controls.Add(title);
            Label sub = new Label(); sub.Text = "Launcher de compatibilidade para jogos x64 — Windows 10/11";
            sub.ForeColor = Color.FromArgb(160, 174, 193); sub.Location = new Point(32, 61); sub.Size = new Size(700, 24); Controls.Add(sub);

            Panel engine = Card(24, 98, 892, 112); Controls.Add(engine); LabelOn(engine, "1. Backend de execução", 16, 12, 400, 24, true);
            backendBox = new ComboBox(); backendBox.DropDownStyle = ComboBoxStyle.DropDownList; backendBox.Location = new Point(17, 46); backendBox.Size = new Size(390, 30);
            backendBox.Items.Add("SDE AVX Lab Runtime (self-test; não inicia jogos)"); backendBox.Items.Add("Intel SDE 9.58 (inicia jogos)"); backendBox.SelectedIndex = 0; backendBox.BackColor = FieldColor; backendBox.ForeColor = Color.White; backendBox.SelectedIndexChanged += BackendChanged; engine.Controls.Add(backendBox);
            backendInfo = LabelOn(engine, "O runtime próprio executa somente o self-test AVX1. Para iniciar o Xenia, selecione Intel SDE 9.58.", 430, 48, 440, 48, false);

            Panel files = Card(24, 222, 892, 178); Controls.Add(files); LabelOn(files, "2. Arquivos e processo", 16, 12, 400, 24, true);
            LabelOn(files, "SDE / executável do backend:", 17, 45, 210, 24, false); sdePath = TextOn(files, 220, 42, 560, 28); ButtonOn(files, "Procurar", 790, 41, 84, 30, BrowseSde, false);
            LabelOn(files, "Jogo ou aplicação (.exe):", 17, 82, 210, 24, false); targetPath = TextOn(files, 220, 79, 560, 28); ButtonOn(files, "Procurar", 790, 78, 84, 30, BrowseTarget, false);
            LabelOn(files, "Diretório de trabalho:", 17, 119, 210, 24, false); workPath = TextOn(files, 220, 116, 560, 28); ButtonOn(files, "Pasta", 790, 115, 84, 30, BrowseWork, false);
            LabelOn(files, "Argumentos do jogo:", 17, 156, 210, 24, false); targetArgs = TextOn(files, 220, 153, 654, 28);

            Panel options = Card(24, 412, 892, 142); Controls.Add(options); LabelOn(options, "3. Perfil e opções", 16, 12, 400, 24, true);
            profileBox = new ComboBox(); profileBox.DropDownStyle = ComboBoxStyle.DropDownList; profileBox.Location = new Point(17, 47); profileBox.Size = new Size(390, 30); profileBox.BackColor = FieldColor; profileBox.ForeColor = Color.White;
            foreach (CpuProfile p in profiles) profileBox.Items.Add(p); profileBox.SelectedIndex = 0; profileBox.SelectedIndexChanged += ProfileChanged; options.Controls.Add(profileBox);
            profileInfo = LabelOn(options, profiles[0].Description, 430, 48, 430, 32, false);
            mixBox = new CheckBox(); mixBox.Text = "Gerar relatório -mix"; mixBox.Location = new Point(20, 90); mixBox.AutoSize = true; mixBox.ForeColor = Color.White; mixBox.Checked = false; options.Controls.Add(mixBox);
            xinuseBox = new CheckBox(); xinuseBox.Text = "-emu-xinuse 0 (experimental)"; xinuseBox.Location = new Point(220, 90); xinuseBox.AutoSize = true; xinuseBox.ForeColor = Color.White; options.Controls.Add(xinuseBox);

            Panel actions = Card(24, 568, 892, 78); Controls.Add(actions);
            ButtonOn(actions, "EXECUTAR", 16, 17, 150, 42, Launch, true);
            ButtonOn(actions, "Salvar perfil", 178, 17, 120, 42, SaveProfile, false);
            ButtonOn(actions, "Carregar perfil", 308, 17, 130, 42, LoadProfile, false);
            ButtonOn(actions, "Diagnóstico", 448, 17, 120, 42, Diagnose, false);
            ButtonOn(actions, "Abrir pasta de logs", 578, 17, 150, 42, OpenLogs, false);
            status = LabelOn(actions, "Pronto.", 744, 24, 135, 30, false); status.ForeColor = Color.FromArgb(115, 210, 145);

            Panel logs = Card(24, 660, 892, 82); Controls.Add(logs); LabelOn(logs, "Eventos", 16, 8, 100, 20, true);
            eventLog = new RichTextBox(); eventLog.Location = new Point(110, 8); eventLog.Size = new Size(764, 62); eventLog.BackColor = Color.FromArgb(9, 12, 17); eventLog.ForeColor = Color.FromArgb(180, 198, 218); eventLog.ReadOnly = true; eventLog.BorderStyle = BorderStyle.FixedSingle; logs.Controls.Add(eventLog);
            BackendChanged(null, EventArgs.Empty);
        }

        private void AutoDetectPaths()
        {
            string here = AppDomain.CurrentDomain.BaseDirectory;
            string[] candidates = {
                Path.GetFullPath(Path.Combine(here, "..", "..", "windows-9.58", "sde.exe")),
                Path.GetFullPath(Path.Combine(here, "..", "..", "intel-sde", "windows-9.58", "sde.exe")),
                Path.GetFullPath(Path.Combine(here, "..", "intel-sde", "windows-9.58", "sde.exe")),
                Path.GetFullPath(Path.Combine(here, "..", "windows-9.58", "sde.exe")),
                @"C:\Sde\sde.exe"
            };
            foreach (string p in candidates) if (File.Exists(p)) { sdePath.Text = p; break; }
            if (String.IsNullOrWhiteSpace(sdePath.Text)) sdePath.Text = candidates[0];
        }

        private void BrowseSde(object sender, EventArgs e) { PickFile(sdePath, "Selecione sde.exe", "sde.exe|sde.exe|Executáveis|*.exe|Todos|*.*"); }
        private void BrowseTarget(object sender, EventArgs e) { PickFile(targetPath, "Selecione o jogo ou programa", "Programas Windows|*.exe|Todos|*.*"); }
        private void PickFile(TextBox target, string title, string filter)
        {
            using (OpenFileDialog d = new OpenFileDialog()) { d.Title = title; d.Filter = filter; d.CheckFileExists = true; if (d.ShowDialog(this) == DialogResult.OK) { target.Text = d.FileName; if (String.IsNullOrWhiteSpace(workPath.Text)) workPath.Text = Path.GetDirectoryName(d.FileName); } }
        }
        private void BrowseWork(object sender, EventArgs e) { using (FolderBrowserDialog d = new FolderBrowserDialog()) if (d.ShowDialog(this) == DialogResult.OK) workPath.Text = d.SelectedPath; }
        private void ProfileChanged(object sender, EventArgs e) { CpuProfile p = profileBox.SelectedItem as CpuProfile; if (p != null) profileInfo.Text = p.Description; }
        private void BackendChanged(object sender, EventArgs e)
        {
            bool external = backendBox != null && backendBox.SelectedIndex == 1;
            if (backendInfo != null) backendInfo.Text = external ? "O Intel SDE externo inicia o aplicativo selecionado com o perfil de CPU escolhido." : "O runtime próprio executa somente o self-test AVX1. Para iniciar o Xenia, selecione Intel SDE 9.58.";
            if (sdePath != null) sdePath.Enabled = external;
            if (targetPath != null) targetPath.Enabled = external;
            if (workPath != null) workPath.Enabled = external;
            if (targetArgs != null) targetArgs.Enabled = external;
            if (mixBox != null) mixBox.Enabled = external;
            if (xinuseBox != null) xinuseBox.Enabled = external;
        }

        private bool ValidateCommon()
        {
            if (String.IsNullOrWhiteSpace(targetPath.Text) || !File.Exists(targetPath.Text)) { MessageBox.Show(this, "Selecione um EXE válido.", "Arquivo ausente", MessageBoxButtons.OK, MessageBoxIcon.Warning); return false; }
            if (String.IsNullOrWhiteSpace(workPath.Text)) workPath.Text = Path.GetDirectoryName(targetPath.Text);
            if (!Directory.Exists(workPath.Text)) { MessageBox.Show(this, "O diretório de trabalho não existe.", "Diretório ausente", MessageBoxButtons.OK, MessageBoxIcon.Warning); return false; }
            return true;
        }

        private void Launch(object sender, EventArgs e)
        {
            if (backendBox.SelectedIndex == 0) { RunIndependentBackend(); return; }
            if (!ValidateCommon()) return;
            if (!File.Exists(sdePath.Text)) { MessageBox.Show(this, "Não encontrei sde.exe. Selecione o arquivo Windows 9.58.", "SDE ausente", MessageBoxButtons.OK, MessageBoxIcon.Warning); return; }
            CpuProfile p = profileBox.SelectedItem as CpuProfile;
            string args = p.Switch;
            if (mixBox.Checked) args += " -mix -mix_format json";
            if (xinuseBox.Checked) args += " -emu-xinuse 0";
            args += " -- " + Quote(targetPath.Text);
            if (!String.IsNullOrWhiteSpace(targetArgs.Text)) args += " " + targetArgs.Text.Trim();
            try
            {
                ProcessStartInfo psi = new ProcessStartInfo(); psi.FileName = sdePath.Text; psi.Arguments = args; psi.WorkingDirectory = workPath.Text; psi.UseShellExecute = false; psi.CreateNoWindow = false;
                Process proc = Process.Start(psi); string line = "SDE iniciado. PID=" + proc.Id + " perfil=" + p.Switch;
                WriteLog(line + " comando=" + sdePath.Text + " " + args); status.Text = "Executando PID " + proc.Id; status.ForeColor = Color.FromArgb(115, 210, 145);
            }
            catch (Exception ex) { WriteLog("Falha ao iniciar: " + ex.Message); status.Text = "Falha ao iniciar"; status.ForeColor = Color.FromArgb(235, 120, 120); MessageBox.Show(this, ex.Message, "Falha ao executar", MessageBoxButtons.OK, MessageBoxIcon.Error); }
        }

        private void RunIndependentBackend()
        {
            string here = AppDomain.CurrentDomain.BaseDirectory;
            string[] candidates = {
                Path.Combine(here, "..", "..", "avx-runtime", "build-windows", "Release", "avx_runtime.exe"),
                Path.Combine(here, "..", "avx_runtime.exe"),
                Path.Combine(here, "avx_runtime.exe")
            };
            string runtimePath = ""; foreach (string p in candidates) if (File.Exists(p)) { runtimePath = Path.GetFullPath(p); break; }
            if (runtimePath == "") { WriteLog("Runtime próprio selecionado, mas avx_runtime.exe não está compilado."); MessageBox.Show(this, "O runtime próprio ainda precisa ser compilado no Windows. Execute avx-runtime\\Build-Windows.bat e tente novamente.", "Runtime experimental", MessageBoxButtons.OK, MessageBoxIcon.Information); return; }
            try { Process proc = Process.Start(new ProcessStartInfo { FileName = runtimePath, Arguments = "--self-test", WorkingDirectory = Path.GetDirectoryName(runtimePath), UseShellExecute = false, CreateNoWindow = false }); if (proc == null) throw new InvalidOperationException("Process.Start returned no process"); WriteLog("Runtime próprio iniciou o self-test. Nenhum jogo foi iniciado. PID=" + proc.Id); status.Text = "Self-test executado"; MessageBox.Show(this, "O runtime próprio ainda não inicia jogos. O self-test AVX1 foi executado.\n\nPara iniciar o Xenia, selecione 'Intel SDE 9.58 (inicia jogos)'.", "Backend experimental", MessageBoxButtons.OK, MessageBoxIcon.Information); } catch (Exception ex) { WriteLog("Falha no runtime: " + ex.Message); }
        }

        private void SaveProfile(object sender, EventArgs e)
        {
            if (!ValidateCommon()) return; string name = Path.GetFileNameWithoutExtension(targetPath.Text); foreach (char c in Path.GetInvalidFileNameChars()) name = name.Replace(c, '_');
            try { string path = Path.Combine(profileDir, name + ".profile"); File.WriteAllText(path, CaptureSettings().Serialize(), Encoding.UTF8); WriteLog("Perfil salvo: " + path); status.Text = "Perfil salvo"; }
            catch (Exception ex) { WriteLog("Falha ao salvar perfil: " + ex.Message); MessageBox.Show(this, ex.Message, "Falha ao salvar perfil", MessageBoxButtons.OK, MessageBoxIcon.Error); }
        }
        private void LoadProfile(object sender, EventArgs e)
        {
            using (OpenFileDialog d = new OpenFileDialog()) { d.InitialDirectory = profileDir; d.Filter = "Perfis SDE AVX Lab|*.profile|Todos|*.*"; if (d.ShowDialog(this) != DialogResult.OK) return; try { Apply(LauncherSettings.Parse(File.ReadAllText(d.FileName, Encoding.UTF8))); WriteLog("Perfil carregado: " + d.FileName); } catch (Exception ex) { WriteLog("Falha ao carregar perfil: " + ex.Message); MessageBox.Show(this, ex.Message, "Falha ao carregar perfil", MessageBoxButtons.OK, MessageBoxIcon.Error); } }
        }
        private LauncherSettings CaptureSettings()
        {
            CpuProfile p = profileBox.SelectedItem as CpuProfile; LauncherSettings s = new LauncherSettings(); s.SdePath = sdePath.Text; s.TargetPath = targetPath.Text; s.WorkingDirectory = workPath.Text; s.Arguments = targetArgs.Text; s.Profile = p == null ? "-snb" : p.Switch; s.Backend = backendBox.Text; s.Mix = mixBox.Checked; s.XInUse = xinuseBox.Checked; return s;
        }
        private void Apply(LauncherSettings s)
        {
            sdePath.Text = s.SdePath; targetPath.Text = s.TargetPath; workPath.Text = s.WorkingDirectory; targetArgs.Text = s.Arguments; mixBox.Checked = s.Mix; xinuseBox.Checked = s.XInUse;
            for (int i = 0; i < profileBox.Items.Count; i++) if (((CpuProfile)profileBox.Items[i]).Switch == s.Profile) profileBox.SelectedIndex = i;
            if (!String.IsNullOrWhiteSpace(s.Backend)) backendBox.SelectedIndex = s.Backend.IndexOf("Intel SDE", StringComparison.OrdinalIgnoreCase) >= 0 ? 1 : 0;
        }
        private void Diagnose(object sender, EventArgs e)
        {
            try { StringBuilder b = new StringBuilder(); b.AppendLine("SDE AVX Lab diagnóstico"); b.AppendLine("Data: " + DateTime.Now); b.AppendLine("OS: " + Environment.OSVersion); b.AppendLine("64-bit OS: " + Environment.Is64BitOperatingSystem); b.AppendLine("SDE: " + sdePath.Text + " => " + File.Exists(sdePath.Text)); b.AppendLine("Jogo: " + targetPath.Text + " => " + File.Exists(targetPath.Text)); b.AppendLine("Backend: " + backendBox.Text); b.AppendLine("Perfil: " + ((CpuProfile)profileBox.SelectedItem).Switch); string path = Path.Combine(appData, "diagnostic-" + DateTime.Now.ToString("yyyyMMdd-HHmmss") + ".txt"); File.WriteAllText(path, b.ToString(), Encoding.UTF8); WriteLog("Diagnóstico salvo: " + path); MessageBox.Show(this, b.ToString() + "\nArquivo: " + path, "Diagnóstico", MessageBoxButtons.OK, MessageBoxIcon.Information); }
            catch (Exception ex) { WriteLog("Falha ao gerar diagnóstico: " + ex.Message); MessageBox.Show(this, ex.Message, "Falha no diagnóstico", MessageBoxButtons.OK, MessageBoxIcon.Error); }
        }
        private void OpenLogs(object sender, EventArgs e) { Directory.CreateDirectory(appData); Process.Start(new ProcessStartInfo { FileName = appData, UseShellExecute = true }); }
        private void WriteLog(string text)
        {
            string line = "[" + DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss") + "] " + text + Environment.NewLine; try { Directory.CreateDirectory(appData); File.AppendAllText(Path.Combine(appData, "launcher.log"), line, Encoding.UTF8); } catch { } if (eventLog != null) { eventLog.AppendText(line); eventLog.SelectionStart = eventLog.Text.Length; eventLog.ScrollToCaret(); }
        }
        private static string Quote(string value) { return "\"" + (value ?? "").Replace("\"", "\\\"") + "\""; }
    }

    internal static class Program
    {
        [STAThread]
        private static void Main() { Application.EnableVisualStyles(); Application.SetCompatibleTextRenderingDefault(false); Application.Run(new MainForm()); }
    }
}
