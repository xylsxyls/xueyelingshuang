-- MySQL dump 10.13  Distrib 5.6.24, for Win32 (x86)
--
-- Host: 127.0.0.1    Database: storagedb
-- ------------------------------------------------------
-- Server version	5.6.24

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Table structure for table `methods`
--

DROP TABLE IF EXISTS `methods`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `methods` (
  `method` varchar(767) NOT NULL,
  PRIMARY KEY (`method`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `methods`
--

LOCK TABLES `methods` WRITE;
/*!40000 ALTER TABLE `methods` DISABLE KEYS */;
INSERT INTO `methods` VALUES ('´îÚ¨¿ªÍ·£¬Å®º¢Õý×¼±¸×öÊ²Ã´£¬ÄãÄÃµÄÕâ¸öÊÇÕâ¼¸ÌìÍÆ³öµÄÐÂÆ·ÖÖ£¬ÎÒ³¢¹ýÁË£¬Î¶µÀ»¹²»´í\r\n½ñÌìÕâ¼Òµê×öµÄ¿¾Èâ²»ÈçÒÔÇ°µÄºÃ³Ô£¬²»¹ýÅ£ÅÅ»¹ÊÇºÜ²»´í£¬Äã¿ÉÒÔ³¢³¢Õâ¸ö'),('´îÚ¨¿ªÍ·£¬Å®º¢Å¿ÔÚ¿¿À¸ÉÏ¿´£¬Äã¿´ÉÏÈ¥ºÜÏ²»¶Ð¡º¢×ÓÂð'),('´îÚ¨¿ªÍ·£¬Å®º¢µÍÍ·¿´ÊÖ»ú»òÕßÒ»¸öÈË×ßÂ·ÌýÒôÀÖ£¬ÄãÉíÉÏÕâ¼þÒÂ·þÕæÆ¯ÁÁ£¬ÔÚÄÄÂòµÄ£¬ÌÔ±¦Âð£¿'),('´îÚ¨¿ªÍ·£¬Å®º¢µ¥×ø£¬ÎÒ¿ÉÒÔ×øÕâÂð£¿'),('µ±¹ÃÄï¿ªÊ¼¾ö¶¨Òª²»ÒªÏ²»¶ÄãµÄÊ±ºò\r\nÎÒÏ²»¶±íÏÖ³öÀ´µÄÑù×ÓÕýÊÇÄãÏ²»¶µÄÑù×Ó'),('½¨½çÖ¸Áî£¬Ã¿ÕÅ¿¨ÅÆÔÚÄÃµ½ÊÖµÄÊ±ºò¶¼ÊÇºÜÆÕÍ¨µÄ£¬¶¼ÐèÒªÅàÑø£¬ÔÚÕýÈ·µÄÅàÓýºó£¬¾Í¿ÉÒÔ±äµÄºÜÓÐ÷ÈÁ¦ºÜÉÁ¹â£¬µ«ÊÇ±¾ÌåÐÔ¸ñÈ´²»ÈÝÒ×¸Ä'),('½¨½çÖ¸Áî£¬Ã¿¸öÈËÔÚ¾Ü¾ø±ðÈËµÄÊ±ºò¶¼»áÁôÏÂÒ»¸öÈ±¿Ú£¬Ö»ÒªÕÒµ½ÄÇ¸öÈ±¿Ú¾Í²»»á±»¾Ü¾ø'),('½¨½çÖ¸Áî£¬ÈçºÎÔ¸Òâ½øÈëÅ®ÉúµÄ¾³£¬ÈËµÄË¼ÏëÊÇ¿ÉÒÔ¸Ä±äµÄ£¬ÈÎºÎÅ®ÉúÎÒ¶¼¿ÉÒÔ½ÓÊÜ£¬²åÈë°ô¿ÉÒÔ¸Ä±äÈÎºÎË¼Ïë'),('½¨½çÖ¸Áî£¬ÎÒÏ²»¶Äã£¬ÎÒÏë¿´×ÅÄã£¬ÎÒ¾ÍÒ»Ö±¿´×ÅÄã'),('½¨½çÖ¸Áî£¬ÎÒ¾ÍÒÔÎÒ×îÕæÊµµÄÒ»ÃæÃæ¶Ô±ðÈË£¬¿´±ðÈË»áÓÐÊ²Ã´ÑùµÄ·´Ó¦'),('½¨½çÖ¸Áî£¬Ì§ÆðÍ·°É£¬¿´¿´ÍâÃæµÄÊÀ½ç'),('½¨½çÖ¸Áî£¬Ì¸ÂÛ¶«Î÷µÄÊ±ºò£¬ÎÒÊÇÕâ·½ÃæµÄ´ïÈË£¬ÎÒÔÚÕâ·½ÃæÓÐºÜ¶à¾­Ñé£¬½ç¿ØÎ´Öª½ç£¬²Â²âËýµÄ½ç£¬¼ÌÐø¿ØËýµÄ½ç£¬²»ÐèÒªËõ½ç'),('½¨½çÖ¸Áî£¬Å®º¢ÎÊÎÊÌâÊ±£¬½øÈëËýÏÖÔÚµÄ¾³£¬»Ø´ð£¬ÎÒÔõÑùÈÃÕâ¸ö×´Ì¬µÄËý¿ªÐÄ'),('½¨½çÖ¸Áî£¬Å®º¢Ö÷¶¯Ê±£¬±íÊ¾ÏÖÔÚµÄÇé¿ö¶ÔÎÒ¸üÓÐÀû£¬¶ø²»ÊÇÏë×ÅÍËÈÃ»òÕß¿¼ÂÇ¸üºÃµÄ'),('½¨½çÖ¸Áî£¬ÄãÈÃÎÒ¸ßÐËÁË£¬ÎÒÏíÊÜÒ»ÏÂ£¬×÷Îª»Ø±¨£¬ÎÒÒ²ÈÃÄã¸ßÐËÒ»ÏÂ'),('½¨½çÖ¸Áî£¬°®±ðÈËÊÇÎªÁËÈÃ×Ô¼ºÊæ·þ£¬ÕâÊÇÒ»ÖÖÍæÀÖµÄÐÂ·½Ê½'),('½¨½çÖ¸Áî£¬±»ÇÆ²»ÆðÊ±Ó¦ÓÐµÄ±íÏÖ£¬µ±±ðÈËÇÆ²»ÆðµÄÊ±ºò£¬³éÏó»¯Ê±¾ÍÊÇ±ðÈË´ÓÄÚÐÄÉì³öÀ´´òÈË£¬ÄÇÃ´±¾Éí´òÈËµÄ×´Ì¬±³ºóµÄÐÄÀï¾ÍÒ»¶¨ÊÇ²»ºÃµÄ£¬¿ÉÒÔ±»Åê»÷£¬±ÈÈçÉúÅÂÊ²Ã´Ê²Ã´£¬ÈçºÎÔÚ´Ë¿ÌÓµÓÐ´òÈËÓûÍû£¬·¢Ð¹ÎÒ¶ÔÕâÒ»ÀàÈËµÄºÞ£¬Äã¾ÍÊÇÕâÒ»ÀàÈËÖÐµÄÒ»¸ö'),('½¨½çÖ¸Áî£¬µ÷Ï·ËýÖ®ºó£¬ÎÒÏíÊÜÄã±»ÎÒµ÷Ï·Ö®ºóµÄ×´Ì¬');
/*!40000 ALTER TABLE `methods` ENABLE KEYS */;
UNLOCK TABLES;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2016-08-23  0:50:05
