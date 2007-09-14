package capture;

import java.io.FileInputStream;
import java.io.IOException;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;

public class Updater implements Runnable {

	//public static char index []= "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	
	public Updater()
	{
		
		Thread t = new Thread(this, "ServerUpdater");
		t.start();
	}
	
	public void run()
	{
		try
		{
			ServerSocket listener = new ServerSocket(7071, 0, 
					InetAddress.getByName((String) ConfigManager.getInstance().getConfigOption("server_listen_address")));
			Socket client;
			System.out.println("Updater: Listening for connections.");
			while (true) {
				client = listener.accept();
				System.out.println("Client connected\n");
				FileInputStream fInput = new FileInputStream((String) ConfigManager.getInstance().getConfigOption("client_latest_version_executable_name"));
				int offset = 0;
				byte[] tempBuf = new byte[1024];
				while (fInput.read(tempBuf, 0, 1024) != -1) {
					client.getOutputStream().write(tempBuf);
					offset += 1024;
				}
				System.out.println("Done sending update\n");
				client.getOutputStream().close();
				fInput.close();
			}
		} catch (IOException e){
			e.printStackTrace();
		}
	}
}
