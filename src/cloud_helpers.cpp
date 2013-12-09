/*
 * File: mecanumbot/src/cloud_helpers.cpp
 * Author: Josh Villbrandt <josh@javconcepts.com>
 * Date: December 2013
 * Description: Various point cloud utilities.
 */

// PCL specific includes
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
// #include <pcl/PCLPointCloud2.h>
#include <pcl/PointIndices.h>
#include <pcl/kdtree/kdtree.h>
#include <pcl/filters/extract_indices.h>
#include <pcl/segmentation/extract_clusters.h>

namespace cloud_helpers {
    
    #define stddev(x, u, s) (x > (u-s) && x < (u+s))

    class Color
    {
        public:
            std::string name;
            // mean value and standard deviation of each color
            uint8_t r_u, r_s;
            uint8_t g_u, g_s;
            uint8_t b_u, b_s;

            Color () :
              r_u (0),
              r_s (0),
              g_u (0),
              g_s (0),
              b_u (0),
              b_s (0)
            {}

            ~Color () {}

            /*void
            ROSInfo()
            {
                std::stringstream ss;
                ss << "(r_u, r_s, g_u, g_s, b_u, b_s): " << name << " ";
                ss << (int)r_u << " " << (int)r_s << " " << (int)g_u;
                ss << " " << (int)g_s << " " << (int)b_u << " " << (int)b_s;
                ROS_INFO("%s", ss.str().c_str());
            }*/
    };

    void
    extractRGB(pcl::PointXYZRGB point, uint8_t &r, uint8_t &g, uint8_t &b)
    {
        uint32_t rgb_val_;
        memcpy(&rgb_val_, &(point.rgb), sizeof(float));
        r = (uint8_t)((rgb_val_ >> 16) & 0x000000ff);
        g = (uint8_t)((rgb_val_ >> 8) & 0x000000ff);
        b = (uint8_t)((rgb_val_) & 0x000000ff);
    }

    std::vector<pcl::PointCloud<pcl::PointXYZRGB>::Ptr>
    segmentByDistance(pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud)
    {
        // Creating the KdTree object and perform Euclidean distance search
        //pcl::KdTree<pcl::PointXYZRGB>::Ptr tree (new pcl::KdTreeFLANN<pcl::PointXYZRGB>);
        pcl::search::KdTree<pcl::PointXYZRGB>::Ptr tree (new pcl::search::KdTree<pcl::PointXYZRGB>);
        tree->setInputCloud (cloud);

        std::vector<pcl::PointIndices> cluster_indices;
        pcl::EuclideanClusterExtraction<pcl::PointXYZRGB> ec;
        ec.setClusterTolerance (0.01); // 1cm
        ec.setMinClusterSize (50);
        ec.setMaxClusterSize (25000);
        ec.setSearchMethod (tree);
        ec.setInputCloud (cloud);
        ec.extract (cluster_indices);

        // Separate clusters
        std::vector<pcl::PointCloud<pcl::PointXYZRGB>::Ptr> clusters;
        for (std::vector<pcl::PointIndices>::const_iterator it = cluster_indices.begin (); it != cluster_indices.end (); ++it)
        {
            pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud_cluster (new pcl::PointCloud<pcl::PointXYZRGB>);
            for (std::vector<int>::const_iterator pit = it->indices.begin (); pit != it->indices.end (); pit++)
                cloud_cluster->points.push_back (cloud->points[*pit]); //
            cloud_cluster->width = cloud_cluster->points.size ();
            cloud_cluster->height = 1;
            cloud_cluster->is_dense = true;
            cloud_cluster->header = cloud->header;

            clusters.push_back(cloud_cluster);
        }

        return clusters;
    }

    pcl::PointIndices::Ptr
    filterColor(pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud, Color color)
    {
        pcl::PointIndices::Ptr indices (new pcl::PointIndices ());
        uint8_t r, g, b;
        for(size_t i = 0; i < cloud->points.size(); i++)
        {
            extractRGB(cloud->points[i], r, g, b);

            // check to see if we are in range
            if(stddev(r, color.r_u, color.r_s) && stddev(g, color.g_u, color.g_s) && stddev(b, color.b_u, color.b_s))
            {
                indices->indices.push_back(i);
            }
        }

        return indices;
    }
}
