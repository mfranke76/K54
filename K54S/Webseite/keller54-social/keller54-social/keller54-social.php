<?php
/**
 * Plugin Name: Keller54 Social-Vorschau
 * Description: Liefert Facebook, WhatsApp & Co. Titel, Text und Vorschaubild (Open Graph) für alle Seiten von keller54.de.
 * Version: 1.0.0
 * Author: Frank E. / Keller54
 */
if ( ! defined( 'ABSPATH' ) ) { exit; }

define( 'K54_SOCIAL_DEFAULT_IMAGE_ID', 122 );

function k54_social_text( $text, $len = 200 ) {
	$text = wp_strip_all_tags( strip_shortcodes( (string) $text ), true );
	$text = trim( preg_replace( '/\s+/u', ' ', $text ) );
	if ( mb_strlen( $text ) > $len ) {
		$text = rtrim( mb_substr( $text, 0, $len - 1 ) ) . '…';
	}
	return $text;
}

add_action( 'wp_head', function () {
	if ( is_admin() || is_feed() ) { return; }

	$site  = get_bloginfo( 'name' );
	$title = $site;
	$desc  = get_bloginfo( 'description' );
	$url   = home_url( add_query_arg( array() ) );
	$img   = 0;
	$type  = 'website';

	if ( is_singular() ) {
		$post  = get_queried_object();
		$url   = get_permalink( $post );
		$title = is_front_page() ? $site : get_the_title( $post ) . ' – ' . $site;
		$custom = get_post_meta( $post->ID, '_k54_og_description', true );
		if ( $custom ) {
			$desc = $custom;
		} elseif ( has_excerpt( $post ) ) {
			$desc = $post->post_excerpt;
		} elseif ( ! is_front_page() ) {
			$desc = get_the_excerpt( $post );
		}
		$img = get_post_thumbnail_id( $post );
		if ( 'post' === $post->post_type ) { $type = 'article'; }
	}
	if ( ! $img ) { $img = K54_SOCIAL_DEFAULT_IMAGE_ID; }
	$desc = k54_social_text( $desc ? $desc : 'Homestudio by Frank E. – Recording, Mixing, Producing und eigene Instrumente aus dem Keller.' );

	$src = wp_get_attachment_image_src( $img, 'full' );

	echo "\n<!-- Keller54 Social-Vorschau -->\n";
	printf( '<meta name="description" content="%s" />' . "\n", esc_attr( $desc ) );
	printf( '<meta property="og:locale" content="de_DE" />' . "\n" );
	printf( '<meta property="og:site_name" content="%s" />' . "\n", esc_attr( $site ) );
	printf( '<meta property="og:type" content="%s" />' . "\n", esc_attr( $type ) );
	printf( '<meta property="og:title" content="%s" />' . "\n", esc_attr( $title ) );
	printf( '<meta property="og:description" content="%s" />' . "\n", esc_attr( $desc ) );
	printf( '<meta property="og:url" content="%s" />' . "\n", esc_url( $url ) );
	if ( $src ) {
		printf( '<meta property="og:image" content="%s" />' . "\n", esc_url( $src[0] ) );
		printf( '<meta property="og:image:width" content="%d" />' . "\n", (int) $src[1] );
		printf( '<meta property="og:image:height" content="%d" />' . "\n", (int) $src[2] );
		printf( '<meta name="twitter:image" content="%s" />' . "\n", esc_url( $src[0] ) );
	}
	echo '<meta name="twitter:card" content="summary_large_image" />' . "\n";
}, 2 );
