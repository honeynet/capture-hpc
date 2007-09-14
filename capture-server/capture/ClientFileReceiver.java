package capture;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;

import sun.misc.BASE64Decoder;

public class ClientFileReceiver {
	private Client client;
	private String fileName;
	private int fileSize;
	private String fileType;
	private BufferedOutputStream outputFile;
	
	public ClientFileReceiver(Client client, Element element) {
		this.client = client;	
		fileName = element.attributes.get("name");
		fileSize = Integer.parseInt(element.attributes.get("size"));
		fileType = element.attributes.get("type");
		if(client.getVisitingUrl() != null)
		{		
			fileName = "log" + File.separator + client.getVisitingUrl().getUrlAsFileName() + "." + fileType;
		}
		try {
			outputFile = new BufferedOutputStream(new FileOutputStream(new File(fileName)));
			this.client.send("<file-accept name=\"" + fileName + "\" />");
		} catch (FileNotFoundException e) {
			e.printStackTrace();
		}	
	}

	public void receiveFilePart(Element element) {
		int partStart = Integer.parseInt(element.attributes.get("part-start"));
		int partEnd = Integer.parseInt(element.attributes.get("part-end"));
		int partSize = partEnd - partStart;
		String encoding = element.attributes.get("encoding");
		if(encoding.equals("base64")) {		
			try {
				BASE64Decoder base64 = new BASE64Decoder();
				byte[] buffer = base64.decodeBuffer(element.data);
				if(buffer.length == partSize)
				{
					outputFile.write(buffer, 0, partSize);
				} else {
					System.err.println("ClientFileReceiver: ERROR part size != decoded size - " + partSize + " != " + buffer.length);
					System.err.println(element.data);
				}
			} catch (IOException e) {
				e.printStackTrace();
			}
		} else {
			System.err.println("ClientFileReceiver: encoding - " + encoding + " not supported");
		}		
	}

	public void receiveFileEnd(Element element) {
		try {
			outputFile.flush();
			outputFile.close();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

}
